#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace dev {

template <typename T>
concept is_single_object = !std::is_array_v<T>;

template <typename T>
concept is_unbounded_array = std::is_unbounded_array_v<T>;

template <typename From, typename To>
concept is_convertible = std::is_convertible_v<From (*)[], To (*)[]>;

// dev::make_unique<T>

template <typename T, typename Deleter = std::default_delete<T>>
class unique_ptr {
public:
    using element_type = std::remove_extent_t<T>;
    using pointer = element_type*;

    // constructors
    unique_ptr() : ptr_(nullptr) {}
    explicit unique_ptr(pointer ptr) : ptr_(ptr) {}
    unique_ptr(unique_ptr&& ptr)
        : ptr_(std::exchange(ptr.ptr_, nullptr)),
          deleter_(std::forward<Deleter>(ptr.deleter_)) {}

    // unique ptr should not be copyable, copy ctor and assignment op is
    // deleted
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(unique_ptr&) = delete;

    // destructor
    ~unique_ptr() { reset(nullptr); }

    // asignment operators
    unique_ptr& operator=(std::nullptr_t) noexcept {
        reset(nullptr);
        return *this;
    }

    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this == &other)
            return *this;
        reset(other.release());
        deleter_ = std::forward<Deleter>(other.deleter_);

        return *this;
    }

    // accessors
    pointer get() const noexcept { return ptr_; }
    Deleter& get_deleter() noexcept { return deleter_; }
    const Deleter& get_deleter() const noexcept { return deleter_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    // return type of operator-> is pointer,
    // return type of operator* is T&,
    pointer operator->() const noexcept
        requires is_single_object<T>
    {
        return ptr_;
    }

    std::add_lvalue_reference_t<element_type> operator*() const
        noexcept(noexcept(*std::declval<pointer>()))
        requires is_single_object<T>
    {
        return *ptr_;
    }

    element_type& operator[](std::size_t i) const
        requires is_unbounded_array<T>
    {
        return get()[i];
    }
    // modifiers
    void reset(pointer ptr = pointer()) noexcept
        requires is_single_object<T>
    {
        reset_helper(ptr);
    }

    template <typename U>
    void reset(U ptr) noexcept
        requires is_unbounded_array<T> && std::is_pointer_v<U> &&
                 is_convertible<std::remove_pointer_t<U>, element_type>
    {
        reset_helper(ptr);
    }

    void reset(std::nullptr_t = nullptr) noexcept
        requires is_unbounded_array<T>
    {
        reset_helper(nullptr);
    }

    [[nodiscard]]
    pointer release() noexcept {
        return std::exchange(ptr_, nullptr);
    }

    void swap(unique_ptr& other) noexcept {
        using std::swap;
        swap(other.ptr_, ptr_);
        swap(other.deleter_, deleter_);
    }

    friend void swap(unique_ptr& first, unique_ptr& second) noexcept {
        first.swap(second);
    }

private:
    template <typename U> void reset_helper(U ptr) noexcept {
        auto old_ptr = get();
        ptr_ = ptr;
        if (old_ptr) {
            get_deleter()(old_ptr);
        }
    }
    pointer ptr_{};

    [[no_unique_address]]
    Deleter deleter_{};
};

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args)
    requires is_single_object<T>
{
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
unique_ptr<T> make_unique(std::size_t size)
    requires is_unbounded_array<T>
{
    return unique_ptr<T>(new std::remove_extent_t<T>[size]);
}
} // end namespace dev
