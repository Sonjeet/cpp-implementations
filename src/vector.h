#pragma once

#include <algorithm>
#include <cstddef>
#include <format>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <utility>

namespace dev {

template <typename T, typename Allocator = std::allocator<T>> class vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    /* constructors */
    /**dsf
     * @brief Default constructor
     */
    vector() = default;

    /**
     * @brief Constructs vector with a fixed capacity
     * @param capacity
     * */
    explicit vector(std::size_t capacity) : capacity_(capacity) {}

    /**
     * @brief Constructs and prefills vector with a value for count number of
     * times
     * @param count
     * @param value
     * @throws bad_alloc or any exception from T's constructor
     * */
    vector(std::size_t count, const T& value)
        : size_(count), capacity_(count), data_(allocate(count)) {
        for (std::size_t i = 0; i < size_; i++) {
            construct(data_ + i, value);
        }
    }

    /**
     * @brief constructs vector with elements from initializer list init
     * @param init
     * @throws bad_alloc or any exception from T's constructor
     * */
    vector(const std::initializer_list<T>& init)
        : size_(init.size()), capacity_(init.size()),
          data_(allocate(capacity_)) {
        auto it = init.begin();
        for (std::size_t i = 0; i < size_; i++) {
            construct(data_ + i, it[i]);
        }
    }

    /**
     * @brief copy constructor - copies elements from the other vector
     * @param other
     * @throws bad_alloc or any exception from T's copy constructor
     * */
    vector(const vector<T>& other)
        : size_(other.size()), capacity_(other.capacity()),
          data_(allocate(capacity_)) {
        for (std::size_t i = 0; i < size_; i++) {
            construct(data_ + i, other[i]);
        }
    }

    /*
     * @brief move constructor moves elements from other vector into this
     * container
     * @param other
     * */
    vector(vector<T>&& other)
        : size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          data_(std::exchange(other.data_, nullptr)) {}

    /**
     * @brief destructor - frees memory and resets size and capacity
     */
    ~vector() noexcept { release(); }

    /* member functions */
    // TODO: assign, assign_range
    Allocator get_allocator() const noexcept { return alloc_; }

    /* element access */

    /*
     * @brief finds element at a position pos
     * @pos - Position of element we want to get
     * @throws std::out_of_range if pos >= size()
     * */
    T& operator[](std::size_t pos) { return at(pos); }

    /*
     * @brief finds element at a position pos
     * @pos - Position of element we want to get
     * @throws std::out_of_range if pos >= size()
     * */
    const T& operator[](std::size_t pos) const { return at(pos); }

    /*
     * @brief finds element at a position pos
     * @pos - Position of element we want to get
     * @throws std::out_of_range if pos >= size()
     * */
    T& at(std::size_t pos) { return data_[pos]; }

    /*
     * @brief finds element at a position pos
     * @pos - Position of element we want to get
     * @throws std::out_of_range if pos >= size()
     * */
    const T& at(std::size_t pos) const { return data_[pos]; }

    /**
     * @brief gets the pointer of the container's data
     * */
    T* data() noexcept { return data_; }

    /**
     * @brief gets the pointer of the container's data
     * */
    T* data() const noexcept { return data_; }

    /*
     * @brief gets the element at the front of the container
     * */
    T& front() { return data_[0]; }

    /*
     * @brief gets the element at the front of the container
     * */
    const T& front() const { return data_[0]; }

    /*
     * @brief gets the element at the back (last element) of the container
     * */
    T& back() { return data_[size_ - 1]; }

    /*
     * @brief gets the element at the back (last element) of the container
     * */
    const T& back() const { return data_[size_ - 1]; }

    /* iterators */
    iterator begin() { return data_; }

    iterator begin() const { return data_; }

    iterator end() { return data_ + size_; }

    iterator end() const { return data_ + size_; }

    /* capacity */
    std::size_t size() const noexcept { return size_; }

    std::size_t capacity() const noexcept { return capacity_; }

    /* modifiers */
    // TODO: insert, insert_range, emplace, erase, append_range, swap

    /**
     * @brief clears the container of any elements it's storing and size becomes
     * zero
     * */
    void clear() noexcept {
        destroy_range(data_, data_ + size_);
        size_ = 0;
    }

    /**
     * @brief inserts an element at the end of the container
     * @param value - Value to insert into end of container
     * @throws whatever emplace_back throws
     * */
    void push_back(const T& value) { emplace_back(value); }

    /**
     * @brief constructs an element in-place at the end of the container
     * @param value - Value to insert into end of container
     * @return element that has been constructed in the container
     * @throws whatever emplace_back throws
     * */
    template <typename... Args> T& emplace_back(Args&&... args) {
        auto pos = emplace_at(size_, std::forward<Args>(args)...);
        return data_[pos];
    }

    /**
     * @brief creates a constructed T at position pos with the args
     * @parma pos - Position to emplace at
     * @param args - arguments to construct object T with
     * @throws whatever shift_realloc and shift_insert may throw
     * */
    template <typename... Args>
    std::size_t emplace_at(std::size_t pos, Args&&... args) {
        // if pos and size same, just construct in-place? with shift insert
        // else we need shift realloc
        if (size_ == capacity_) {
            shift_realloc(pos, std::forward<Args>(args)...);
        } else {
            shift_insert(pos, std::forward<Args>(args)...);
        }
        size_++;
        return pos;
    }

    void pop_back() { destroy(data_ + --size_); }

    void resize(std::size_t count) { resize_helper(count); }

    void resize(std::size_t count, const T& value) {
        resize_helper(count, value);
    }

    void shrink_to_fit() {
        if (size_ == capacity_)
            return;
        reallocate(size_);
    }

    /* non-member functions */
private:
    using allocator_traits = std::allocator_traits<Allocator>;
    /*
     * allocate
     * deallocate
     * destroy
     * destroy_range
     * construct
     * reallocate
     * relocate into
     * shift_insert - when we need to insert at pos, but things to rhs need
     * shfting shift_reallocate - similar motive as shift_insert but realloc
     * needed since cap too small
     */

    template <typename... Args>
    void resize_helper(std::size_t count, Args&&... args) {
        if (count <= size_) {
            destroy_range(data_ + count, data_ + size_);
            size_ = count;
            return;
        }

        if (count > capacity_) {
            reallocate(std::max(count, grow_capacity()));
        }

        const auto old_size = size_;
        try {
            // we have a new cap, construct until count elements?
            for (; size_ < count; ++size_)
                construct(data_ + size_, std::forward<Args>(args)...);

        } catch (...) {
            destroy_range(data_ + old_size, data_ + size_);
            size_ = old_size;
        }
    }

    /**
     * @brief allocates memory for new_cap elements
     * @param new_cap Capcaity of the allocated container
     * @return Pointer to the allocated memory
     * @throws std::bad_alloc if allocation fails
     */
    T* allocate(std::size_t new_cap) {
        if (!new_cap)
            return nullptr;
        return allocator_traits::allocate(alloc_, new_cap);
    }

    /*
     * @brief deallocates the memory at @p data
     * @param data Pointer to the memory that should be deallocated
     * @param cap - Number of elems the memory was allocated with
     * */
    void deallocate(T* data, std::size_t cap) noexcept {
        if (!data)
            return;
        allocator_traits::deallocate(alloc_, data, cap);
    }

    /**
     * @brief destroys the element at the slot
     * @param slot Poiinter to the slot of memory that will be destructed
     */
    void destroy(T* slot) noexcept { allocator_traits::destroy(alloc_, slot); }

    /**
     * @brief destroys a range of elements [first, last)
     * @param first Start of the range, this element is inclusive of the
     * destruction
     * @param last End of the range, this element is not inclusive of the
     * destruction
     */
    void destroy_range(T* first, T* last) noexcept {
        while (last != first) {
            destroy(--last);
        }
    }

    /*
     * @brief constructs an object T in the memory slot by forwarding args into
     * T's constructor
     * @param slot - The slot at which T will be constructed
     * @param args - The arguments to be forwarded to T's constructor for
     * construction in-place
     * */
    template <typename... Args> void construct(T* slot, Args&&... args) {
        allocator_traits::construct(alloc_, slot, std::forward<Args>(args)...);
    }

    /*
     * @brief allocates a new buffer, moves data from old buffer into new buffer
     * and establishes this as the class's allocated memory buffer
     * @param new_cap - capacity of the newly allocated buffer
     * @throws Whatever allocate throws
     * */
    void reallocate(std::size_t new_cap) {
        // new allocator
        // try reloate else deallocate and throw
        // replace buffer
        T* new_mem = allocate(new_cap);

        try {
            relocate(new_mem, data_, size_);
        } catch (...) {
            deallocate(new_mem, new_cap);
            throw;
        }
        replace_buffer(new_mem, new_cap);
    }

    /* @brief deletes the existing buffer and replaces with the buffer new_data
     * @param new_data - the new buffer for the class
     * @param new_cap - capacity of new buffer
     * */
    void replace_buffer(T* new_data, std::size_t new_cap) noexcept {
        // destroy existing
        // deallocate existing
        // update buffer and cap_ with new_data and new_cap
        destroy_range(data_, data_ + size_);
        deallocate(data_, capacity_);
        data_ = new_data;
        capacity_ = new_cap;
    }

    /* @brief relocates data from a source buffer into a destination buffer
     * @param dest - destination buffer
     * @param src - source buffer
     * @param size - number of elements that need relocating to the new buffer
     * */
    void relocate(T* dest, T* src, std::size_t size) {
        // try move, else destroy and throw
        // store counter of how many objects built, useful for when we need to
        // know num to destroy
        std::size_t number_of_relocations = 0;
        try {
            for (std::size_t i = 0; i < size; i++) {
                construct(dest + i, std::move_if_noexcept(src[i]));
                number_of_relocations++;
            }
        } catch (...) {
            destroy_range(dest, dest + number_of_relocations);
            throw;
        }
    }

    /*
     * @brief destroys and deallocates any objects and allocated memory
     * */
    void release() noexcept {
        destroy_range(data_, data_ + size_);
        deallocate(data_, capacity_);
        data_ = nullptr;
        size_ = capacity_ = 0;
    }

    /*
     * @brief inserts an object T in-place at a position, shifts existing
     * elements from that position onwards to make room for the new object T
     * @param pos - position at which object T will be inserted
     * @param args - arguments to construct object T with
     * */
    template <typename... Args>
    void shift_insert(std::size_t pos, Args&&... args) {
        // if size is pos then just insert
        // construct + shift element at position size
        // shift elements one forward
        // insert element at position pos
        if (pos == size_) {
            construct(data_ + size_, std::forward<Args>(args)...);
            return;
        }
        T tmp(std::forward<Args>(args)...);
        construct(data_ + size_, std::move(data_[size_ - 1]));
        for (std::size_t i = size_ - 1; i > pos; i--) {
            data_[i] = std::move(data_[i - 1]);
        }
        data_[pos] = std::move(tmp);
    }

    /*
     * @brief allocate a new memory, insert an object T into the position and
     * relocate data from the existing buffer into the newly allocated one.
     * since a new object has taken up position pos, exisiting elements from
     * position pos and onwards must be shifted to the memory slot to the right
     * @param pos - The position at which to insert an object T
     * @param args - arguments to construct object T with
     */
    template <typename... Args>
    void shift_realloc(std::size_t pos, Args&&... args) {
        // alloc new mem
        // construct obj T
        // move stuff on lhs
        // move stuff on rhs
        // insert T
        // if throws at any point, destroy and deallocate new mem
        auto new_cap = grow_capacity();
        auto new_mem = allocate(new_cap);
        bool constructed_new_val = false;
        bool relocated_lhs = false;

        try {
            // move stuff in lhs
            // move stuff in rhs
            // construct tmp into mem
            construct(new_mem + pos, std::forward<Args>(args)...);
            constructed_new_val = true;

            relocate(new_mem, data_, pos);
            relocated_lhs = true;

            relocate(new_mem + pos + 1, data_ + pos, size_ - pos);
        } catch (...) {
            if (constructed_new_val)
                destroy(new_mem + pos);
            if (relocated_lhs)
                destroy_range(new_mem, new_mem + pos);
            deallocate(new_mem, new_cap);
            throw;
        }

        replace_buffer(new_mem, new_cap);
    }

    std::size_t grow_capacity() const noexcept {
        return capacity_ == 0 ? 1 : capacity_ << 1;
    }

    [[no_unique_address]] Allocator alloc_;
    std::size_t size_{0};
    std::size_t capacity_{0};
    T* data_{nullptr};
};
} // namespace dev
