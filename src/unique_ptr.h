#include <cassert>
#include <memory>

namespace dev {

template <typename T, typename Deleter = std::default_delete<T>>
class unique_ptr {
public:
    using pointer = T*;
    using element_type = T;
    using deleter_type = Deleter;

    unique_ptr() noexcept : ptr_(nullptr) {}

    explicit unique_ptr(pointer p) noexcept : ptr_(p) {}

    unique_ptr(unique_ptr&& p) noexcept
        : ptr_(std::exchange(p.ptr_, nullptr)),
          deleter_(std::forward<Deleter>(p.deleter_)) {}

    unique_ptr(const unique_ptr& p) = delete;
    ~unique_ptr() { reset(nullptr); }

    unique_ptr& operator=(unique_ptr&& r) noexcept {
        reset(nullptr);
        ptr_ = std::exchange(r.ptr_, nullptr);

        return *this;
    }
    unique_ptr& operator=(std::nullptr_t) noexcept { reset(nullptr); }
    unique_ptr& operator=(const unique_ptr& r) = delete;

    /*
     * Modifiers
     * */
    [[nodiscard]] pointer release() noexcept {
        return std::exchange(ptr_, nullptr);
    }
    void reset(pointer ptr = pointer()) noexcept {
        if (!ptr_)
            return;

        get_deleter()(ptr_);
        ptr_ = ptr;
    }
    void reset(std::nullptr_t = nullptr) noexcept {
        if (!ptr_)
            return;

        get_deleter()(ptr_);
        ptr_ = nullptr;
    }
    void swap(unique_ptr& other) noexcept {
        std::swap(other.ptr_, ptr_);
        std::swap(other.deleter_, deleter_);
    }

    /*
     * Observers
     * */
    pointer get() const noexcept { return ptr_; }
    Deleter& get_deleter() noexcept { return deleter_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    /*
     * Single-object version
     * */
    // why do we need the lvalue ref type - what does it help with/prevent
    // essentially askin why can we not do T&
    // conditional noexcept? so noexcept only if the deref op to our type is
    // labelled noexcept the below signature at a basic level is T& operator*()
    // noexcept [conditional noexcept though]
    std::add_lvalue_reference_t<T> operator*() const
        noexcept(noexcept(*std::declval<T>())) {
        // we want to deref the ptr
        // any preconditions? can't deref a nullptr
        // return the deref'd pointer to this object
        assert(ptr_ != nullptr && "operator* running on empty unique_ptr");
        return *ptr_;
    }
    pointer operator->() const noexcept {
        // what is the op doing
        assert(ptr_ != nullptr && "operator-> running on empty unique_ptr");
        return ptr_;
    }
    element_type& operator[](std::size_t i) const {
        assert(ptr_ != nullptr && "operator[] running on empty unique_ptr");
        return ptr_[i];
    }

private:
    pointer ptr_{};
    [[no_unique_address]] Deleter deleter_{};
};

} // namespace dev
