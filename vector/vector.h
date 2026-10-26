#include <cstddef>
#include <format>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <utility>

namespace dev {

template <typename T, typename Allocator = std::allocator<T>> class vector {
public:
    /* constructors */
    vector() = default;

    /* member functions */

    /* element access */

    /* iterators */

    /* capacity */

    /* modifiers */

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
     * shift_insert - when we need to insert at pos, but things to rhs need shfting
     * shift_reallocate - similar motive as shift_insert but realloc needed since cap too small
     */

    /**
     * @brief allocates memory for new_cap elements
     * @param new_cap Capcaity of the allocated container
     * @return Pointer to the allocated memory
     * @throws std::length_error if new_cap exceeds max_size()
     * @throws std::bad_alloc if allocation fails
     */
    T* allocate(std::size_t new_cap) { return allocator_traits::allocate(alloc_, new_cap); }

    /*
     * @brief deallocates the memory at @p data
     * @param data Pointer to the memory that should be deallocated
     * @param cap - Number of elems the memory was allocated with
     * */
    void deallocate(T* data, std::size_t cap) noexcept {
        allocator_traits::deallocate(alloc_, data, cap);
    }

    /**
     * @brief destroys the element at the slot
     * @param slot Poiinter to the slot of memory that will be destructed
     */
    void destroy(T* slot) noexcept { allocator_traits::destroy(alloc_, slot); }

    /**
     * @brief destroys a range of elements [first, last)
     * @param first Start of the range, this element is inclusive of the destruction
     * @param last End of the range, this element is not inclusive of the destruction
     */
    void destroy_range(T* first, T* last) noexcept {
        while (last != first) {
            destroy(--last);
        }
    }

    /*
     * @brief constructs an object T in the memory slot by forwarding args into T's constructor
     * @param slot - The slot at which T will be constructed
     * @param args - The arguments to be forwarded to T's constructor for construction in-place
     * */
    template <typename... Args> void construct(T* slot, Args&&... args) {
        allocator_traits::construct(alloc_, slot, std::forward<Args&&>(args)...);
    }

    /*
     * @brief allocates a new buffer, moves data from old buffer into new buffer and establishes
     * this as the class's allocated memory buffer
     * @param new_cap - capacity of the newly allocated buffer
     * @throws Whatever allocate throws
     * */
    void reallocate(std::size_t new_cap) {
        // new allocator
        // try reloate else deallocate and throw
        // replace buffer
    }

    /* @brief deletes the existing buffer and replaces with the buffer new_data
     * @param new_data - the new buffer for the class
     * @param new_cap - capacity of new buffer
     * */
    void replace_buffer(T* new_data, std::size_t new_cap) {
        // destroy existing
        // deallocate existing
        // update buffer and cap_ with new_data and new_cap
    }

    /* @brief relocates data from a source buffer into a destination buffer
     * @param dest - destination buffer
     * @param src - source buffer
     * @param size - number of elements that need relocating to the new buffer
     * */
    void relocate(T* dest, T* src, std::size_t size) {
        // try move, else destroy and throw
        // store counter of how many objects built, useful for when we need to know num to destroy
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
     * @brief inserts an object T in-place at a position, shifts existing elements from that
     * position onwards to make room for the new object T
     * @param pos - position at which object T will be inserted
     * @param args - arguments to construct object T with
     * */
    template <typename... Args> void shift_insert(std::size_t pos, Args&&... args) {
        // if size is pos then just insert
        // construct + shift element at position size
        // shift elements one forward
        // insert element at position pos
    }

    /*
     * @brief allocate a new memory, insert an object T into the position and relocate
     * data from the existing buffer into the newly allocated one. since a new object has
     * taken up position pos, exisiting elements from position pos and onwards must be
     * shifted to the memory slot to the right
     * @param pos - The position at which to insert an object T
     * @param args - arguments to construct object T with
     */
    template <typename... Args> void shift_realloc(std::size_t pos, Args&&... args) {
        // alloc new mem
        // construct obj T
        // move stuff on lhs
        // move stuff on rhs
        // insert T
        // if throws at any point, destroy and deallocate new mem
    }

    // TODO: not correct, taking up space, look into no_unique_address
    [[no_unique_address]] Allocator alloc_;
    std::size_t size_{0};
    std::size_t capacity_{0};
    T* data_{nullptr};
};
} // namespace dev
