# `dev::vector` bug report

Bugs found in the methods that currently exist in `vector.h` that the unit
tests in `tests/vector_test.cpp` do not catch, with a fix for each.

How this was verified:

- Each bug was reproduced against the current `vector.h` with a standalone
  program built with `-fsanitize=address,undefined` (Apple clang 21, C++23).
- All fixes below were applied together to a copy of `vector.h`, the same
  reproductions were re-run, and the existing GoogleTest suite was run against
  the fixed header under ASan/UBSan. Results are in [Verification](#verification).

The hunks compose into a single patch: applying every hunk in this document
produces the verified header. Some hunks fix several issues; they are shown
once and referenced from the other issues.

## Summary

| #  | Issue | Category | Fix |
|----|-------|----------|-----|
| 1  | Growth from capacity 0 overflows the buffer | Memory safety | [Shared helpers](#shared-fix-growth-helpers), [#1](#1-growth-from-capacity-0-overflows-the-buffer) |
| 2  | `operator=` overflows when `other` is larger | Memory safety | [#2/#3](#2-operator-overflows-when-other-is-larger) |
| 3  | `operator=` destroys elements twice | Memory safety | [#2/#3](#2-operator-overflows-when-other-is-larger) |
| 4  | `insert` writes one slot past the end | Memory safety | [#4/#5/#8](#4-insert-writes-one-slot-past-the-end) |
| 5  | `insert` returns a dangling iterator after growth | Memory safety | [#4/#5/#8](#4-insert-writes-one-slot-past-the-end) |
| 6  | `push_back(v[0])` reads freed memory when full | Memory safety | [#6/#15](#6-push_backv0-reads-freed-memory-when-full) |
| 7  | `pop_back()` on empty underflows `size_` | Memory safety (UB in `std::vector` too) | [#7](#7-pop_back-on-empty-underflows-size_) |
| 8  | `insert` produces wrong contents | Correctness | [#4/#5/#8](#4-insert-writes-one-slot-past-the-end) |
| 9  | `push_front` without growth overwrites element 0 | Correctness | [#9](#9-push_front-without-growth-overwrites-element-0) |
| 10 | `vector(count)` creates no elements | Correctness | [#10](#10-vectorcount-creates-no-elements) |
| 11 | `const` vectors can be mutated | Correctness | [#11](#11-const-vectors-can-be-mutated) |
| 12 | Growth never destroys old elements (leak) | Leak / UB | [Shared helpers](#shared-fix-growth-helpers) |
| 13 | `deallocate` called with the wrong size | UB | [Shared helpers](#shared-fix-growth-helpers) |
| 14 | Failed allocation leaves the vector corrupted | Exception safety | [Shared helpers](#shared-fix-growth-helpers) |
| 15 | `emplace_back` copies args and builds a temporary | Correctness / performance | [#6/#15](#6-push_backv0-reads-freed-memory-when-full) |
| 16 | Copy/move/assign take `vector<T>`, breaking custom allocators | Memory safety | [#16](#16-copymoveassign-take-vectort-breaking-custom-allocators) |

---

## Shared fix: growth helpers

Fixes #12, #13 and #14, and is the base for #1, #4–#6, #8, #9 and #15.

`push_back`, `emplace_back`, `insert` and `push_front` each had their own copy
of the growth code, and all four copies had the same three defects:

- **#12:** old elements are moved from but never destroyed. Only
  `deallocate_all()` is called, never `destroy_all()`.
- **#13:** `capacity_` is doubled *before* `deallocate_all()`, so the old
  buffer is freed as if it had the new capacity. Passing the wrong `n` to
  `deallocate` is UB.
- **#14:** `capacity_` is modified *before* `allocate()`. If allocation throws,
  the vector reports a capacity it does not have.

The fix replaces the four copies with private helpers that allocate first,
then relocate, then destroy and free the old buffer using its own capacity.
`deallocate_all()` also resets `capacity_`, so a freed buffer is never
described as still having capacity.

```diff
 #include <memory>
 #include <stdexcept>
+#include <utility>
```

```diff
 private:
+    std::size_t next_capacity() const { return capacity_ == 0 ? 1 : capacity_ * 2; }
+
+    // Move the live elements into new_mem (capacity new_cap), then destroy the
+    // old elements and free the old buffer using its *own* capacity.
+    void relocate_into(T* new_mem, std::size_t new_cap) {
+        for (std::size_t i = 0; i < size_; i++) {
+            std::construct_at(new_mem + i, std::move_if_noexcept(data_[i]));
+        }
+        destroy_all();
+        deallocate_all();
+        data_ = new_mem;
+        capacity_ = new_cap;
+    }
+
+    void reallocate(std::size_t new_cap) {
+        T* new_mem = allocate(new_cap); // may throw: *this untouched so far
+        relocate_into(new_mem, new_cap);
+    }
+
     T* allocate(std::size_t capacity) {
```

```diff
     void deallocate_all() {
         if (!capacity_)
             return;

         Allocator alloc;
         alloc.deallocate(data_, capacity_);
         data_ = nullptr;
+        capacity_ = 0;
     }
```

`std::move_if_noexcept` moves elements when their move constructor is
`noexcept` and copies them otherwise. With a throwing move, a failure halfway
through relocation would leave elements in both buffers half-moved; copying
leaves the originals intact.

---

## 1. Growth from capacity 0 overflows the buffer

**Problem:** growth used `capacity_ <<= 1` (or `<<= 2`), which leaves 0
unchanged. The code then allocates 0 elements and constructs into them.
Capacity is 0 after a move, after `vector(0, x)`, and after `shrink_to_fit()`
on an empty vector.

**Repro:**

```cpp
dev::vector<int> a{{1, 2}};
dev::vector<int> b(std::move(a));
a.push_back(7);              // ASan: heap-buffer-overflow

dev::vector<int> v{{1}};
v.pop_back();
v.shrink_to_fit();
v.push_back(1);              // ASan: heap-buffer-overflow
```

**Fix:** `next_capacity()` in the [shared helpers](#shared-fix-growth-helpers)
maps 0 to 1. `emplace_back` and `insert` use it (see #6 and #4). Separately,
`shrink_to_fit()` on an empty vector released nothing and allocated a
zero-sized buffer that was never freed, because `deallocate_all()` skips
capacity 0:

```diff
     void shrink_to_fit() {
         if (capacity_ == size_)
             return;
+        if (size_ == 0) {
+            delete_all(); // don't allocate a zero-sized buffer
+            return;
+        }
```

---

## 2. `operator=` overflows when `other` is larger

Also fixes **#3: `operator=` destroys elements twice.**

**Problem:**

- **#2:** the grow branch calls `allocate(capacity_)` with the *old*, smaller
  capacity and never updates `capacity_`, then copies all of `other` into it.
  The condition also checks `other.capacity()` when what matters is
  `other.size()`.
- **#3:** `destroy_all()` runs once at the top and again in the `else` branch.
  By then `size_` already holds `other.size()`, so the second call destroys
  objects that are already dead, or slots that never held an object.

The existing `copy_assignment` test does not run `operator=`:
`dev::vector<int> vec = other;` is copy *construction*. The
`copy_assignment_with_smaller_vector` test uses `int`, which has no destructor,
so the double destroy is invisible.

**Repro:**

```cpp
dev::vector<int> small{{1}};
dev::vector<int> big{{1, 2, 3, 4, 5, 6, 7, 8}};
small = big;                 // ASan: heap-buffer-overflow

const std::string L(40, 'x');   // long enough to heap-allocate
dev::vector<std::string> a{{L, L, L}};
dev::vector<std::string> b{{L}};
a = b;                       // ASan: attempting double-free
```

**Fix:** destroy once via `clear()`, size the buffer by `other.size()`,
record the new capacity, and set `size_` only after the copies exist.

```diff
-    vector<T>& operator=(const vector<T>& other) {
+    vector& operator=(const vector& other) {
         if (this == &other)
             return *this;

-        destroy_all();
-        size_ = other.size();
+        clear(); // destroy current elements exactly once

-        if (other.capacity() > capacity()) {
+        if (other.size() > capacity()) {
             deallocate_all();
-            data_ = allocate(capacity_);
-        } else {
-            destroy_all();
+            data_ = allocate(other.size());
+            capacity_ = other.size();
         }

-        for (std::size_t i = 0; i < size_; i++) {
+        for (std::size_t i = 0; i < other.size(); i++) {
             std::construct_at(data_ + i, other[i]);
         }
+        size_ = other.size();

         return *this;
     }
```

(The `vector<T>` → `vector` change in the signature is #16.)

---

## 4. `insert` writes one slot past the end

Also fixes **#5: `insert` returns a dangling iterator after growth** and
**#8: `insert` produces wrong contents**, plus the aliasing case of #6 for
`insert`.

**Problem:**

- **#4:** the shift loop runs `i <= size_`, so it reads `data_[size_]`
  (uninitialised) and writes `data_[size_ + 1]`. When `size_ + 1 == capacity_`
  there is no growth, and that write is past the end of the buffer.
- **#8:** the shift runs front to back, so each step overwrites the next
  element with a copy of the previous one. It also `construct_at`s over live
  objects without destroying them first.
- **#5:** it returns the original `pos`. After reallocation, `pos` points into
  the freed buffer.
- **#6 (insert):** `value` may refer to an element of this vector, which is
  freed by reallocation or overwritten by the shift.

The `insert_at_pos` test only checks `size()`.

**Repro:**

```cpp
dev::vector<int> v{{1, 2, 3, 4, 5}};
v.reserve(10);
v.insert(v.begin() + 1, 9);  // contents: 1 9 2 2 2 2   (expected 1 9 2 3 4 5)

dev::vector<int> w{{1, 2, 3}};
w.reserve(4);
w.insert(w.begin(), 0);      // ASan: heap-buffer-overflow

dev::vector<int> x{{1, 2, 3}};
auto it = x.insert(x.begin() + 1, 9);
*it;                         // ASan: heap-use-after-free
```

**Fix:** copy `value` first, grow with the shared helper, then shift back to
front. Slot `size_` is raw memory, so it is constructed into. The other slots
hold live objects, so they are move-assigned. Return an iterator computed from
the index.

```diff
     iterator insert(iterator pos, const T& value) {
-        /*
-         * check the capacity - grow if needed
-         * back to front, move the elemnents to pos+1
-         * in pos itself, deref and then assign the venue there
-         * */
-        std::size_t pos_idx = pos - begin();
-        if (size_ + 1 > capacity_) {
-            capacity_ <<= 1;
-            auto new_mem = allocate(capacity_);
-            for (std::size_t i = 0; i < size_; i++) {
-                std::construct_at(new_mem + i, std::move(data_[i]));
-            }
+        const std::size_t pos_idx = pos - begin();
+        T tmp(value); // copy first: value may refer into this vector

-            deallocate_all();
-            data_ = new_mem;
+        if (size_ == capacity_) {
+            reallocate(next_capacity());
         }

-        for (std::size_t i = pos_idx; i <= size_; i++) {
-            std::construct_at(data_ + i + 1, std::move(data_[i]));
+        if (pos_idx == size_) {
+            std::construct_at(data_ + size_, std::move(tmp));
+        } else {
+            // Slot size_ is raw memory: construct into it. The rest are live:
+            // shift back to front with move-assignment.
+            std::construct_at(data_ + size_, std::move(data_[size_ - 1]));
+            for (std::size_t i = size_ - 1; i > pos_idx; i--) {
+                data_[i] = std::move(data_[i - 1]);
+            }
+            data_[pos_idx] = std::move(tmp);
         }
-        std::construct_at(data_ + pos_idx, value);
         ++size_;

-        return pos;
+        return begin() + pos_idx; // pos is stale if we reallocated
     }
```

Note: the shift now requires `T` to be move-assignable. `std::vector::insert`
has the same requirement.

---

## 6. `push_back(v[0])` reads freed memory when full

Also fixes **#15: `emplace_back` copies args and builds a temporary**.

**Problem:**

- **#6:** when the vector is full, `push_back` frees the old buffer before it
  copies `value`. If `value` refers into that buffer, as in `v.push_back(v[0])`,
  the copy reads freed memory. For `std::string` there was no ASan report, but
  the new element is copied from the moved-from (empty) original.
- **#15:** `emplace_back(Args... args)` takes its arguments by value, which
  copies them. It then builds a temporary `T(...)` and move-constructs from
  it. That is not in-place construction, and it fails for non-movable `T`.

**Repro:**

```cpp
dev::vector<int> v{{42}};
v.push_back(v[0]);           // ASan: heap-use-after-free

dev::vector<std::string> s{{std::string(40, 'x')}};
s.push_back(s[0]);           // s[1].size() == 0, expected 40
```

**Fix:** forwarding references, construct directly in the slot, and when
growing, construct the new element into the new buffer *before* relocating
and freeing the old one. `push_back` delegates to `emplace_back` so the growth
logic exists in one place.

```diff
-    template <typename... Args> void emplace_back(Args... args) {
-        if (size_ + 1 > capacity_) {
-            capacity_ <<= 1;
-            auto new_mem = allocate(capacity_);
-            for (std::size_t i = 0; i < size_; i++) {
-                std::construct_at(new_mem + i, std::move(data_[i]));
-            }
-            deallocate_all();
-            data_ = new_mem;
+    template <typename... Args> void emplace_back(Args&&... args) {
+        if (size_ == capacity_) {
+            const std::size_t new_cap = next_capacity();
+            T* new_mem = allocate(new_cap);
+            // Build the new element before touching the old buffer: args may
+            // refer into it, e.g. v.emplace_back(v[0]).
+            std::construct_at(new_mem + size_, std::forward<Args>(args)...);
+            relocate_into(new_mem, new_cap);
+        } else {
+            std::construct_at(data_ + size_, std::forward<Args>(args)...);
         }
-
-        std::construct_at(data_ + size_++, T(std::forward<Args>(args)...));
+        ++size_;
     }

-    void push_back(const T& value) {
-        if (size_ + 1 > capacity_) {
-            capacity_ <<= 1;
-            auto new_mem = allocate(capacity_);
-            for (std::size_t i = 0; i < size_; i++) {
-                std::construct_at(new_mem + i, std::move(data_[i]));
-            }
-            deallocate_all();
-            data_ = new_mem;
-        }
-
-        std::construct_at(data_ + size_++, value);
-    }
+    void push_back(const T& value) { emplace_back(value); }
```

---

## 7. `pop_back()` on empty underflows `size_`

**Problem:** `--size_` wraps to `SIZE_MAX`, and `destroy_at` runs on an
address far outside the buffer. `std::vector` leaves this as UB too, so this
fix is about catching mistakes, not about matching the standard.

**Repro:**

```cpp
dev::vector<int> v{{1}};
v.pop_back();
v.pop_back();                // UBSan: pointer-overflow
```

**Fix:** a debug-build assertion. It compiles away under `NDEBUG`, so release
builds behave like `std::vector`.

```diff
+#include <cassert>
 #include <cstddef>
```

```diff
-    void pop_back() { std::destroy_at(data_ + --size_); }
+    void pop_back() {
+        assert(!empty() && "pop_back() on empty vector");
+        std::destroy_at(data_ + --size_);
+    }
```

---

## 9. `push_front` without growth overwrites element 0

**Problem:** the no-growth path never shifts. It `construct_at`s over the
live element 0, and `size_++` exposes an uninitialised last slot. The growth
path also had all the [shared growth defects](#shared-fix-growth-helpers). The
`push_front` test only covers the growth path, because `{1..5}` starts full.

**Repro:**

```cpp
dev::vector<int> v{{1, 2, 3}};
v.reserve(10);
v.push_front(0);             // contents: 0 2 3 <garbage>   (expected 0 1 2 3)
```

**Fix:** `push_front` is `insert` at `begin()`, so delegate to it and inherit
the fixes from #4.

```diff
-    void push_front(const T& value) {
-        /*
-         * might need to allocate new block
-         * need to move existing elements one space forward
-         * if more cap needed, create new mem and move to there
-         * if existing cap okay then, traverse backward, and move element 1 ahead
-         * */
-        if (size_ + 1 > capacity_) {
-            capacity_ <<= 2;
-            auto new_mem = allocate(capacity_);
-            for (std::size_t i = 0; i < size_; i++) {
-                std::construct_at(new_mem + i + 1, std::move(data_[i]));
-            }
-            deallocate_all();
-            data_ = new_mem;
-        }
-
-        std::construct_at(data_, value);
-        size_++;
-    }
+    void push_front(const T& value) { insert(begin(), value); }
```

---

## 10. `vector(count)` creates no elements

**Problem:** `vector<int>(5)` gives `size() == 0, capacity() == 5`.
`std::vector` gives 5 value-initialised elements. The default constructor
delegated to `vector(1)`, so fixing the count constructor alone would make a
default-constructed vector contain one element.

**Repro:**

```cpp
dev::vector<int> v(5);       // size=0 cap=5   (expected size=5, all zero)
```

**Fix:** value-initialise `count` elements, and make the default constructor
an empty, unallocated vector (capacity 0, which `next_capacity()` from the
[shared helpers](#shared-fix-growth-helpers) now handles).

```diff
-    vector() : vector(1) {}
+    vector() = default;
```

```diff
-    explicit vector(std::size_t count) : capacity_(count), size_(0), data_(allocate(count)) {}
+    explicit vector(std::size_t count) : capacity_(count), size_(count), data_(allocate(count)) {
+        for (std::size_t i = 0; i < size_; i++) {
+            std::construct_at(data_ + i); // value-initialise
+        }
+    }
```

The `shrink_to_fit` test still expects capacity 8 after six `push_back`s on a
default-constructed vector. That still holds: the capacities go 1, 2, 4, 8.

---

## 11. `const` vectors can be mutated

**Problem:** `at`, `operator[]`, `front`, `back` and `data` are `const` member
functions but return non-const `T&` / `T*`, so writes through a
`const dev::vector&` compile.

**Repro:**

```cpp
void f(const dev::vector<int>& v) {
    v.at(0) = 99;   // all compile today
    v[0] = 1;
    v.front() = 2;
}
```

**Fix:** a non-const and a const overload for each. The bounds check moves to
a private helper so the two `at` overloads share it.

```diff
-    T& at(std::size_t pos) const {
-        if (pos >= size())
-            throw std::out_of_range(
-                std::format("pos {0} is out of bounds for vector of size {1}", pos, size()));
+    T& at(std::size_t pos) {
+        check_bounds(pos);
+        return data_[pos];
+    }
+    const T& at(std::size_t pos) const {
+        check_bounds(pos);
         return data_[pos];
     }

-    T& operator[](std::size_t pos) const { return at(pos); }
+    T& operator[](std::size_t pos) { return at(pos); }
+    const T& operator[](std::size_t pos) const { return at(pos); }

-    T& front() const { return data_[0]; }
+    T& front() { return data_[0]; }
+    const T& front() const { return data_[0]; }

-    T& back() const { return data_[size() - 1]; }
+    T& back() { return data_[size() - 1]; }
+    const T& back() const { return data_[size() - 1]; }
```

```diff
-    T* data() const { return data_; }
+    T* data() { return data_; }
+    const T* data() const { return data_; }
```

```diff
 private:
+    void check_bounds(std::size_t pos) const {
+        if (pos >= size())
+            throw std::out_of_range(
+                std::format("pos {0} is out of bounds for vector of size {1}", pos, size()));
+    }
+
```

---

## 12, 13, 14. Leak, wrong deallocation size, corruption on failed allocation

Fixed by the [shared growth helpers](#shared-fix-growth-helpers).

**Repro (#12):** with a counting type that has no move constructor, so
`std::move` falls back to copying:

```cpp
struct Tracked {
    static inline int alive = 0;
    Tracked(int = 0) { ++alive; }
    Tracked(const Tracked&) { ++alive; }
    ~Tracked() { --alive; }
};

{
    dev::vector<Tracked> v{{1, 2}};
    v.push_back(3);
}
// Tracked::alive == 2   (expected 0)
```

**#13** is not detectable with ASan on this setup, because Apple clang builds
with `-fno-sized-deallocation`, so the wrong size never reaches the heap
checker. A counting allocator detects it: the allocated element count does not
return to zero (see [Verification](#verification)).

---

## 16. Copy/move/assign take `vector<T>`, breaking custom allocators

**Problem:** inside the class, `vector<T>` means `vector<T, std::allocator<T>>`.
For any other `Allocator`, these are not the copy constructor, move
constructor and copy assignment of *this* type. The compiler then generates
its own memberwise versions, which copy the `data_` pointer, so two vectors
share one buffer and both free it.

**Repro:**

```cpp
dev::vector<std::string, CountingAlloc<std::string>> a{{std::string(40, 'x')}};
dev::vector<std::string, CountingAlloc<std::string>> b(a);  // shallow copy
// ASan: DEADLYSIGNAL when a and b are destroyed
```

**Fix:** use the injected class name `vector`, which means
`vector<T, Allocator>` inside the class.

```diff
-    vector(const vector<T>& other) : size_(other.size()), capacity_(other.capacity()) {
+    vector(const vector& other) : size_(other.size()), capacity_(other.capacity()) {
```

```diff
-    vector(vector<T>&& other)
+    vector(vector&& other)
```

The `operator=` signature change is included in the [#2/#3 hunk](#2-operator-overflows-when-other-is-larger).

---

## Verification

### Reproductions

| Scenario | Before | After |
|----------|--------|-------|
| `push_back` on moved-from vector (#1) | heap-buffer-overflow | `7` |
| `shrink_to_fit()` on empty, then `push_back` (#1) | heap-buffer-overflow | `1` |
| `small = big` (#2) | heap-buffer-overflow | `1 2 3 4 5 6 7 8` |
| `a = b` with long strings (#3) | attempting double-free | size 1, capacity 3 |
| `insert` at `begin()` with `size_ + 1 == capacity_` (#4) | heap-buffer-overflow | `0 1 2 3` |
| Dereference iterator returned by `insert` after growth (#5) | heap-use-after-free | `9` |
| `insert(begin(), v[1])` with strings (#6) | — | `v[0] == "b"` |
| `push_back(v[0])` on full `vector<int>` (#6) | heap-use-after-free | `42 42` |
| `push_back(v[0])` on full `vector<std::string>` (#6) | new element empty | 40 characters |
| `pop_back()` twice on one-element vector (#7) | UBSan pointer-overflow | `Assertion failed: (!empty() && "pop_back() on empty vector")` |
| `insert(begin() + 1, 9)` into `{1,2,3,4,5}` (#8) | `1 9 2 2 2 2` | `1 9 2 3 4 5` |
| `push_front(0)` with spare capacity (#9) | `0 2 3 <garbage>` | `0 1 2 3` |
| `push_front(0)` when full (#9) | — | `0 1 2 3` |
| `vector<int>(5)` (#10) | size 0, capacity 5 | size 5, capacity 5, elements 0 |
| Writes through `const dev::vector&` (#11) | 0 compile errors | 5 compile errors |
| `Tracked` alive after grow, `insert`, `push_front` (#12) | 2 | 0 |
| Counting allocator: elements outstanding after mixed ops (#13) | attempting double-free | 0 outstanding, 0 null frees |
| Copy and assign with a custom allocator (#16) | DEADLYSIGNAL | correct copies |

### Existing test suite

With the fixed header, built with `-fsanitize=address,undefined`:

```
100% tests passed, 0 tests failed out of 14
```

### Not fixed in this patch

These are outside the scope of the bugs listed above:

- **Strong exception guarantee:** if an element's copy or move constructor
  throws partway through relocation, or the new element's constructor throws
  in `emplace_back`'s growth path, the new buffer leaks. Handling this needs
  cleanup of the partially built buffer before rethrowing.
- **Copy constructor of an empty vector:** it calls `allocate(0)` and then
  never frees the result, because capacity is 0.
- **Compiler warnings:**
  - `-Wreorder-ctor` in the constructors whose initializer lists don't follow
    member declaration order. The behaviour is correct as written.
  - Unused `Allocator alloc` in `destroy_all()`.
