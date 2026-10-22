#include <cstddef>
#include <format>
#include <initializer_list>
#include <memory>
#include <stdexcept>

namespace dev {

/**
 * constructors:
 *       copy
 *       assignment
 *       move?
 *       initialiser list
 *  what is rule of five? applicable here?
 *  destructor: deallocate gracefully
 *  shrink_to_fit - reduced capacity to whatever the current size is? is it determined by compiler?
 *  push_back - add value to end if there is capacity
 *  emplace_back - TODO: research
 *  push_front - if capacity, shift everything by one, then add val to empty space in beginning
 *  reserve - deallocate existing mem, alloc a reserved mem amount, then move elements there
 *  clear - must keep capacity? destroy elements that used to exist?
 *  insert, emplace - research
 *  resize - allocate more mem, or just deallocate and destroy all excess mem
 *  pop_back - just destroy the tings cuz
 *  swap - std swap the tings cuz
 *  front, back - get the front and back
 *  iterators - research best impl approach
 *  swap - swap the tings
 *
 *
 *  operator_overloads:
 *       assignment,
 *       copy
 *       index
 */

template <typename T, typename Allocator = std::allocator<T>> class vector {
public:
    using iterator = T*;
    using reverse_iterator = std::reverse_iterator<iterator>;

    vector() : vector(1) {}

    /** member functions
     * assign, assign_range
     * get_allocator
     *
     * */
    explicit vector(std::size_t count) : capacity_(count), size_(0), data_(allocate(count)) {}
    vector(const vector<T>& other) : size_(other.size()), capacity_(other.capacity()) {
        data_ = allocate(capacity_);
        for (std::size_t i = 0; i < size(); i++) {
            std::construct_at(data_ + i, other[i]);
        }
    }
    vector(vector<T>&& other)
        : data_(std::exchange(other.data_, nullptr)), size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)) {}
    vector(std::initializer_list<T> other) : capacity_(other.size()), size_(other.size()) {
        data_ = allocate(capacity_);
        std::size_t i = 0;
        for (const auto& value : other) {
            std::construct_at(data_ + i++, value);
        }
    }

    vector(std::size_t count, T value) : capacity_(count), size_(count) {
        data_ = allocate(capacity_);
        for (std::size_t i = 0; i < capacity_; i++) {
            std::construct_at(data_ + i, value);
        }
    }

    // TODO: conditional - if new cap is bigger?
    // if new cap is bigger, allocate more mem, move stuff there, deallocate prev
    //
    vector<T>& operator=(const vector<T>& other) {
        if (this == &other)
            return *this;

        destroy_all();
        size_ = other.size();

        if (other.capacity() > capacity()) {
            deallocate_all();
            data_ = allocate(capacity_);
        } else {
            destroy_all();
        }

        for (std::size_t i = 0; i < size_; i++) {
            std::construct_at(data_ + i, other[i]);
        }

        return *this;
    }
    /*
     * destructor
     * */
    ~vector() { delete_all(); }

    /**
     * element access
     * index op
     * front
     * back
     * data
     */
    T& at(std::size_t pos) const {
        if (pos >= size())
            throw std::out_of_range(
                std::format("pos {0} is out of bounds for vector of size {1}", pos, size()));
        return data_[pos];
    }

    T& operator[](std::size_t pos) const { return at(pos); }

    T& front() const { return data_[0]; }

    T& back() const { return data_[size() - 1]; }

    /**
     * iterators
     * begin, cbegin, end, cend, rbegin, crbegin, rend, crend
     */

    /*
     * capacity:
     * max_size
     * reserve
     * capacity
     * shrink_to_fit
     *
     * */
    void reserve(std::size_t new_cap) {
        if (new_cap <= capacity_)
            return;

        auto new_mem = allocate(new_cap);
        auto new_size = size_;
        for (auto i = 0; i < new_size; i++) {
            std::construct_at(new_mem + i, std::move(data_[i]));
        }
        delete_all();
        data_ = new_mem;
        capacity_ = new_cap;
        size_ = new_size;
    }

    bool empty() const { return size() == 0; }

    std::size_t size() { return size_; }

    std::size_t capacity() const { return capacity_; }
    /*
     * capacity
     * */

    /**
     * modifiers
     * resize
     * swap
     */
    void clear() {
        destroy_all();
        size_ = 0;
    }

    template <typename... Args> void emplace_back(Args... args) {
        if (size_ + 1 > capacity_) {
            capacity_ <<= 1;
            auto new_mem = allocate(capacity_);
            for (std::size_t i = 0; i < size_; i++) {
                std::construct_at(new_mem + i, std::move(data_[i]));
            }
            deallocate_all();
            data_ = new_mem;
        }

        std::construct_at(data_ + size_++, T(std::forward<Args>(args)...));
    }

    void push_back(const T& value) {
        if (size_ + 1 > capacity_) {
            capacity_ <<= 1;
            auto new_mem = allocate(capacity_);
            for (std::size_t i = 0; i < size_; i++) {
                std::construct_at(new_mem + i, std::move(data_[i]));
            }
            deallocate_all();
            data_ = new_mem;
        }

        std::construct_at(data_ + size_++, value);
    }

    iterator insert(iterator pos, const T& value) {
        /*
         * check the capacity - grow if needed
         * back to front, move the elemnents to pos+1
         * in pos itself, deref and then assign the venue there
         * */
        std::size_t pos_idx = pos - begin();
        if (size_ + 1 > capacity_) {
            capacity_ <<= 1;
            auto new_mem = allocate(capacity_);
            for (std::size_t i = 0; i < size_; i++) {
                std::construct_at(new_mem + i, std::move(data_[i]));
            }

            deallocate_all();
            data_ = new_mem;
        }

        for (std::size_t i = pos_idx; i <= size_; i++) {
            std::construct_at(data_ + i + 1, std::move(data_[i]));
        }
        std::construct_at(data_ + pos_idx, value);
        ++size_;

        return pos;
    }

    void push_front(const T& value) {
        /*
         * might need to allocate new block
         * need to move existing elements one space forward
         * if more cap needed, create new mem and move to there
         * if existing cap okay then, traverse backward, and move element 1 ahead
         * */
        if (size_ + 1 > capacity_) {
            capacity_ <<= 2;
            auto new_mem = allocate(capacity_);
            for (std::size_t i = 0; i < size_; i++) {
                std::construct_at(new_mem + i + 1, std::move(data_[i]));
            }
            deallocate_all();
            data_ = new_mem;
        }

        std::construct_at(data_, value);
        size_++;
    }

    void pop_back() { std::destroy_at(data_ + --size_); }

    void shrink_to_fit() {
        if (capacity_ == size_)
            return;

        auto new_capacity = size_;
        auto new_mem = allocate(new_capacity);
        for (auto i = 0; i < new_capacity; i++) {
            std::construct_at(new_mem + i, std::move(data_[i]));
        }

        delete_all();
        data_ = new_mem;
        size_ = capacity_ = new_capacity;
    }

    T* data() const { return data_; }
    std::size_t size() const { return size_; }

    /**
     * iterator methods
     * begin, end, rbegin, rend
     */
    iterator begin() { return data_; }

    iterator end() { return data_ + size_; }

    reverse_iterator rbegin() { return reverse_iterator(end()); }

    reverse_iterator rend() { return reverse_iterator(begin()); }

private:
    T* allocate(std::size_t capacity) {
        Allocator alloc;
        return alloc.allocate(capacity);
    }

    void delete_all() {
        destroy_all();
        deallocate_all();
        size_ = capacity_ = 0;
    }

    void deallocate_all() {
        if (!capacity_)
            return;

        Allocator alloc;
        alloc.deallocate(data_, capacity_);
        data_ = nullptr;
    }

    void destroy_all() noexcept {
        if (!capacity_)
            return;

        Allocator alloc;
        for (auto i = 0; i < size(); i++) {
            std::destroy_at(data_ + i);
        }
    }

    std::size_t capacity_{};
    std::size_t size_{};
    T* data_{};
};
} // namespace dev
