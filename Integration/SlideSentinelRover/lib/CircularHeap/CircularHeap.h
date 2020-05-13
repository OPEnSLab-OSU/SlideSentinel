//
// Created by Noah on 4/17/2020.
//

#ifndef SLIDESENTINELROVER_CIRCULARHEAP_H
#define SLIDESENTINELROVER_CIRCULARHEAP_H

#include <type_traits>
#include <new>
#include "CircularBuffer.h"

/**
 * @brief A ring-buffer implementation with arbitrary element size.]
 *
 * CircularHeap is a ring-buffer implementation designed to be used
 * as an efficient and memory-safe FIFO queue for elements of an
 * arbitrary size. Only the top element of the CircularHeap can be
 * accesses/removed, behaving like an opaque queue structure.
 * CircularHeap can store any type, but relies on the user to
 * memorize the order of types in the CircularHeap for access.
 * In addition, all types must be trivially destructible,
 * as CircularHeap does not call any destructors.
 *
 * The combination of the above assumptions allows CircularHeap to
 * efficiently allocate memory using a ring-buffer, creating both
 * a fast and safe method of arbitrarily storing data in a FIFO
 * fashion. This structure is ideal for the event system commonly
 * used my state machines.
 *
 * @note The value returned by CircularHeap::size may be larger
 * than the sum of the sizes of the objects in the CircularHeap.
 * This is because of two reasons:
 *  1. when large objects are allocated the buffer
 * may not have enough storage at the end to construct the whole
 * object: when this happens CircularHeap will pad the size of
 * the buffer until it wraps to the start of the memory block,
 * resulting in a larger allocated size.
 *  2. Objects must have a minimum size (specified by min_alignment)
 *  to prevent unintentional unaligned access. Objects are padded
 *  until this minimum alignment is reached.
 *
 * @tparam max_size The number of bytes allocated towards the heap, must be a power of two due to alignment constraints
 * @tparam min_alignment The minimum size for a type, used to prevent unaligned access.
 */
template<size_t max_size, size_t min_alignment = 4>
class CircularHeap {
    static_assert((max_size& (max_size - 1)) == 0 && max_size != 0, "max_size must be a power of 2!");
public:
    CircularHeap()
            : m_array{}
            , m_length(0)
            , m_start(0)
            , m_last_right_pad(0)
            , m_obj_sizes() {}

    void clear() { m_length = 0; m_start = 0; m_last_right_pad = 0; m_obj_sizes.reset(); }

    ~CircularHeap() = default;
    CircularHeap& operator=(CircularHeap& rhs) = delete;

    /**
     * Returns the number of bytes used by allocated memory
     * (may be larger than the sum of the sizes of the allocated objects).
     */
    size_t size() const { return m_length; }
    /** Returns the number of objects currently in the heap. */
    size_t count() const { return m_obj_sizes.size(); }
    /** Returns the bytes allocated for this heap in total. */
    size_t max() const { return max_size; }

    bool full() const { return m_length == max_size; }
    bool empty() const { return m_length == 0; }

    /**
     * Allocate space for and construct an object into the CircularHeap.
     * @tparam T Type to allocate, must be trivially destructible.
     * @tparam Args Arguments to pass to the constructor of T.
     * @param args Arguments to pass to the constructor of T.
     * @return True if the object was constructed, false if there
     * was not enough memory available.
     */
    template<typename T, typename ...Args>
    bool allocate_push_back(Args&& ... args);

    /** Returns a pointer to the front of the queue, or nullptr if the queue is empty */
    const void* get_front() const {
        if (empty())
            return nullptr;
        return &m_array[m_start];
    }

    /**
     * "Deallocates" (read: overwrites) the item at the front of the queue.
     */
    void deallocate_pop_front();
private:

    static size_t m_get_true_index(const size_t index, const size_t m_start) {
        const auto corIndex = m_start + index;
        return corIndex >= max_size ? corIndex - max_size : corIndex;
    }

    static size_t m_align(const size_t type_size) {
        return type_size % 4 == 0 ? type_size : type_size + 4 - (type_size % 4);
    }

    alignas(max_size) unsigned char m_array[max_size];
    size_t m_length;
    size_t m_start;
    size_t m_last_right_pad;
    CircularBuffer<size_t, 64> m_obj_sizes;
};

template<size_t max_size, size_t min_alignment>
template<typename T, typename ...Args>
bool CircularHeap<max_size, min_alignment>::allocate_push_back(Args&& ... args) {
    static_assert(std::is_trivially_destructible<T>::value, "Values in circular heap must only not have destructors!");
    // make sure the size of T is word aligned to prevent hard faults
    const size_t tsize = m_align(sizeof(T));
    // if we're out of space, return
    if (tsize > (max() - size()) || m_obj_sizes.full())
        return false;
    // if the type is larger than the aligned size we have at the end,
    // skip to the beginning
    if (m_start + m_length < max_size
        && tsize >  max_size - (m_start + m_length)) {
        // if we wouldn't have space anyway, oh well
        if (tsize > m_start)
            return false;
        // else increase the length to start at the begining
        if (m_length > 0) {
            m_last_right_pad = max_size - (m_start + m_length);
            m_length += m_last_right_pad;
        }
        else
            m_start = 0;
    }
    // get the pointer
    T* ret = reinterpret_cast<T*>(&m_array[m_get_true_index(m_length, m_start)]);
    // construct a new T obj at the pointer
    new (ret) T(args...);
    // add the size of T to the length
    m_length += tsize;
    // and to the object size tracker
    m_obj_sizes.add_back(tsize);
    // done!
    return true;
}

template<size_t max_size, size_t min_alignment>
void CircularHeap<max_size, min_alignment>::deallocate_pop_front() {
    if (empty())
        return;
    const size_t tsize = m_obj_sizes.front();
    if (tsize > m_length) {
        // TODO: error?
        return;
    }
    // move the start forward to account for the newly freed memory,
    m_start += tsize;
    // decrement the length
    m_length -= tsize;
    // check if we need to account for right padding
    // by resetting the start to zero
    if (m_length == 0
        || (m_last_right_pad > 0
            && m_start >= (max_size - m_last_right_pad))) {
        // if there's a length, correct for right pad
        // else just set the start to zero
        if (m_length != 0) {
            if (m_last_right_pad > m_length) {
                // TODO: error?
            }
            else
                m_length -= m_last_right_pad;
        }
        m_start = 0;
        m_last_right_pad = 0;
    }
        // else wrap if needed
    else if (m_start >= max_size)
        m_start -= max_size;
    // pop the front of the size tracker
    m_obj_sizes.destroy_front();
}

#endif //SLIDESENTINELROVER_CIRCULARHEAP_H