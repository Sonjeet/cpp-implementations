#pragma once

// dev::shared_ptr without weak_ptr support.
//
// Same scope as sptr.h minus weak_ptr, shared_ptr(const weak_ptr&) and
// enable_shared_from_this. With no weak_ptrs, the object and the control
// block always die at the same moment (when the count reaches 0), which
// removes the weak count and the dispose()/destroy() split.
//
// Defines the same names as sptr.h, so include one or the other, not both.

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory> // std::default_delete
#include <type_traits>
#include <utility>

#include "unique_ptr.h" // dev::unique_ptr, for shared_ptr(unique_ptr&&)

namespace dev {

template <typename T> class shared_ptr;

namespace detail {

// ---------------------------------------------------------------------------
// Control block
// ---------------------------------------------------------------------------
//
// One count: the number of shared_ptrs. When it reaches 0 the block deletes
// itself, and the derived block's destructor destroys the object.
class control_block_base {
public:
    void add_ref() noexcept {
        // relaxed: the caller already holds a reference, so the count
        // can't reach 0 during this increment
        count_.fetch_add(1, std::memory_order_relaxed);
    }

    void release() noexcept {
        // acq_rel: release publishes this owner's writes to the object;
        // acquire (on the last decrement) sees every other owner's writes
        // before the destructor runs
        if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete this; // virtual destructor -> the derived block's cleanup
    }

    long use_count() const noexcept {
        return count_.load(std::memory_order_relaxed);
    }

    virtual ~control_block_base() = default;

private:
    std::atomic<long> count_{1};
};

// Block for shared_ptr(Y*, D): the object lives elsewhere.
//
// This class is where the type erasure happens. It is a template on the
// real type Y and the deleter D, but shared_ptr<T> only stores a
// control_block_base*. So shared_ptr<Base> built from a Derived* still
// deletes a Derived, and the deleter's type isn't part of shared_ptr's type.
template <typename Y, typename D>
class cb_ptr final : public control_block_base {
public:
    cb_ptr(Y* p, D d) : ptr_(p), deleter_(std::move(d)) {}
    ~cb_ptr() override { deleter_(ptr_); }

private:
    Y* ptr_;
    [[no_unique_address]] D deleter_;
};

// Block for make_shared<T>: the object lives inside the block, so there is
// one allocation instead of two. It's an ordinary member here, because
// without weak_ptrs the object never needs to die before the block does.
template <typename T> class cb_inplace final : public control_block_base {
public:
    template <typename... Args>
    explicit cb_inplace(Args&&... args) : value_(std::forward<Args>(args)...) {}

    T* get() noexcept { return &value_; }

private:
    T value_;
};

// Tag for the private constructor that takes over a count the caller has
// already set (used by make_shared).
struct adopt_t {};
inline constexpr adopt_t adopt{};

// Y* can be stored as T* (Derived -> Base, adding const, anything -> void).
template <typename Y, typename T>
concept compatible_ptr = std::is_convertible_v<Y*, T*>;

} // namespace detail

// ---------------------------------------------------------------------------
// shared_ptr
// ---------------------------------------------------------------------------
template <typename T> class shared_ptr {
    static_assert(!std::is_array_v<T>, "shared_ptr<T[]> is out of scope");

public:
    using element_type = T;

    // ----- constructors ----------------------------------------------------

    constexpr shared_ptr() noexcept = default;
    constexpr shared_ptr(std::nullptr_t) noexcept {}

    // Y is deduced from the argument, so the block deletes a Y, not a T.
    //   shared_ptr<Base> p(new Derived);   // Y = Derived, runs ~Derived
    template <typename Y>
        requires detail::compatible_ptr<Y, T>
    explicit shared_ptr(Y* p) : shared_ptr(p, std::default_delete<Y>{}) {}

    template <typename Y, typename D>
        requires detail::compatible_ptr<Y, T> && std::invocable<D&, Y*>
    shared_ptr(Y* p, D d) : ptr_(p) {
        try {
            cb_ = new detail::cb_ptr<Y, D>(p, std::move(d));
        } catch (...) {
            d(p); // nothing owns p yet, so free it before rethrowing
            throw;
        }
    }

    shared_ptr(const shared_ptr& other) noexcept
        : ptr_(other.ptr_), cb_(other.cb_) {
        if (cb_)
            cb_->add_ref();
    }

    template <typename Y>
        requires detail::compatible_ptr<Y, T>
    shared_ptr(const shared_ptr<Y>& other) noexcept
        : ptr_(other.ptr_), cb_(other.cb_) {
        if (cb_)
            cb_->add_ref();
    }

    // Moving transfers the reference, so the count doesn't change.
    shared_ptr(shared_ptr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)),
          cb_(std::exchange(other.cb_, nullptr)) {}

    template <typename Y>
        requires detail::compatible_ptr<Y, T>
    shared_ptr(shared_ptr<Y>&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)),
          cb_(std::exchange(other.cb_, nullptr)) {}

    // Aliasing: share owner's control block, but get() returns p.
    // Typically p points into *owner, e.g. at a member.
    template <typename Y>
    shared_ptr(const shared_ptr<Y>& owner, element_type* p) noexcept
        : ptr_(p), cb_(owner.cb_) {
        if (cb_)
            cb_->add_ref();
    }

    // Takes over u's object and deleter. If allocating the block throws,
    // u still owns the object (release() hasn't been called yet).
    template <typename Y, typename D>
        requires detail::compatible_ptr<Y, T>
    shared_ptr(dev::unique_ptr<Y, D>&& u) {
        if (!u)
            return;
        Y* p = u.get();
        cb_ = new detail::cb_ptr<Y, D>(p, std::move(u.get_deleter()));
        ptr_ = p;
        (void)u.release();
    }

    ~shared_ptr() {
        if (cb_)
            cb_->release();
    }

    // ----- assignment ------------------------------------------------------
    //
    // Copy-and-swap: the temporary takes the new value (incrementing its
    // count), swaps with *this, then releases the old value when it is
    // destroyed. Increment-before-decrement makes `p = p` and
    // `p = p->next` safe without a self-assignment check.

    shared_ptr& operator=(const shared_ptr& other) noexcept {
        shared_ptr(other).swap(*this);
        return *this;
    }

    template <typename Y>
        requires detail::compatible_ptr<Y, T>
    shared_ptr& operator=(const shared_ptr<Y>& other) noexcept {
        shared_ptr(other).swap(*this);
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& other) noexcept {
        shared_ptr(std::move(other)).swap(*this);
        return *this;
    }

    template <typename Y>
        requires detail::compatible_ptr<Y, T>
    shared_ptr& operator=(shared_ptr<Y>&& other) noexcept {
        shared_ptr(std::move(other)).swap(*this);
        return *this;
    }

    // ----- modifiers -------------------------------------------------------

    void reset() noexcept { shared_ptr().swap(*this); }

    template <typename Y>
        requires detail::compatible_ptr<Y, T>
    void reset(Y* p) {
        shared_ptr(p).swap(*this);
    }

    template <typename Y, typename D>
        requires detail::compatible_ptr<Y, T> && std::invocable<D&, Y*>
    void reset(Y* p, D d) {
        shared_ptr(p, std::move(d)).swap(*this);
    }

    void swap(shared_ptr& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(cb_, other.cb_);
    }

    friend void swap(shared_ptr& a, shared_ptr& b) noexcept { a.swap(b); }

    // ----- observers -------------------------------------------------------

    T* get() const noexcept { return ptr_; }

    // add_lvalue_reference_t so the declaration is valid for shared_ptr<void>
    std::add_lvalue_reference_t<T> operator*() const noexcept
        requires(!std::is_void_v<T>)
    {
        return *ptr_;
    }

    T* operator->() const noexcept { return ptr_; }

    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    // Approximate under concurrency: another thread can change it right after.
    long use_count() const noexcept { return cb_ ? cb_->use_count() : 0; }

private:
    template <typename> friend class shared_ptr;
    template <typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args&&... args);

    // Takes ownership of a reference the caller has already counted.
    shared_ptr(detail::adopt_t, T* p, detail::control_block_base* cb) noexcept
        : ptr_(p), cb_(cb) {}

    T* ptr_ = nullptr;                         // what get() returns
    detail::control_block_base* cb_ = nullptr; // what gets deleted, and when
};

// ---------------------------------------------------------------------------
// Non-member functions
// ---------------------------------------------------------------------------

// One allocation for the block and the object. If T's constructor throws,
// `new` frees the block automatically.
template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
    static_assert(!std::is_array_v<T>, "make_shared<T[]> is out of scope");
    auto* cb = new detail::cb_inplace<T>(std::forward<Args>(args)...);
    return shared_ptr<T>(detail::adopt, cb->get(), cb);
}

// Both casts are the aliasing constructor with a converted pointer.
template <typename T, typename U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U>& r) noexcept {
    return shared_ptr<T>(r, static_cast<T*>(r.get()));
}

// A failed cast returns an empty shared_ptr, not one that shares ownership
// while holding nullptr.
template <typename T, typename U>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& r) noexcept {
    if (auto* p = dynamic_cast<T*>(r.get()))
        return shared_ptr<T>(r, p);
    return {};
}

// C++20 generates !=, and the reversed forms (nullptr == p), from these.
template <typename T, typename U>
bool operator==(const shared_ptr<T>& a, const shared_ptr<U>& b) noexcept {
    return a.get() == b.get();
}

template <typename T>
bool operator==(const shared_ptr<T>& a, std::nullptr_t) noexcept {
    return !a;
}

} // namespace dev

/**
 *
 *  Read it in this order rather than top to bottom. The file is laid out for
  the compiler, which needs the helpers defined first, but the ideas make
  more sense starting from what a shared_ptr holds.

  1. The two data members at the bottom of shared_ptr: T* ptr_ and
  control_block_base* cb_. Everything else in the file manages these two
  pointers.

  2. control_block_base. This is the single count, with three operations:
  add_ref (relaxed), release (acq_rel, then delete this at zero), and
  use_count. Check the comments on why each memory order is used.

  3. cb_ptr<Y, D>. This is the type erasure. It's a template on the real
  type Y and the deleter D, but shared_ptr only sees the base pointer. Its
  destructor calls the deleter.

  4. The raw-pointer constructors shared_ptr(Y*) and shared_ptr(Y*, D). This
  is where a block gets created. Look for Y being deduced (why ~Derived
  runs), and for the try/catch that frees p if the allocation throws.

  5. The copy constructor, move constructor and ~shared_ptr. These are the
  three ways the count is used: copying increments it, moving leaves it
  unchanged, and destroying calls release.

  6. Assignment and reset. They're all copy-and-swap. Work out on paper why
  p = p is safe without a self-assignment check.

  7. make_shared, cb_inplace and adopt_t. This is the second kind of block,
  with the object stored inside it. adopt is the private constructor that
  takes over a block whose count is already 1.

  8. The converting constructors, the aliasing constructor, then the two
  casts. The converting ones are the copy and move constructors with a
  Derived* → Base* conversion. The aliasing constructor shares cb_ but
  stores a different ptr_. The casts are the aliasing constructor with a
  converted pointer.

  9. shared_ptr(dev::unique_ptr&&). It uses cb_ptr again. Note that it
  allocates the block before calling release().

  10. The observers and operator==. These are trivial, so skim them.

  Steps 1–6 make a complete, working shared_ptr on their own, and they're
  roughly what you'd write in a 45-minute interview. Steps 7–10 are the
  extras from the scope list.

 */
