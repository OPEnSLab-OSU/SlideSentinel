#pragma once
#include <stdlib.h>

template<size_t Len, size_t Align>
struct aligned_storage {
	struct type {
		alignas(Align) unsigned char data[Len];
	};
};

#ifdef ARDUINO_ARCH_AVR
// Default placement versions of operator new.
inline void* operator new(size_t, void* __p) throw() { return __p; }
inline void* operator new[](size_t, void* __p) throw() { return __p; }

// Default placement versions of operator delete.
inline void  operator delete  (void*, void*) throw() { }
inline void  operator delete[](void*, void*) throw() {}
#else
#include <new>
#endif
/**
 * Simple fixed-size ringbuffer implementation with no heap allocation.
 * Useful for implementing a queue or deque without creating heap
 * fragmentation. For more information on ring buffers, please see
 * https://en.wikipedia.org/wiki/Circular_buffer.
 * @tparam T The type to populate the ringbuffer with
 * @tparam max_size The maximum number of elements allowed in the buffer.
 */
template<typename T, size_t max_size>
class CircularBuffer {
public:

	// create an aligned array type 
	using array_t = typename aligned_storage<sizeof(T), alignof(T)>::type;

	CircularBuffer<T, max_size>()
		: m_array{}
		, m_length(0)
		, m_start(0) {}

	CircularBuffer<T, max_size>(const CircularBuffer& rhs)
		: m_array{}
		, m_length(0)
		, m_start(0) {
		for (const auto& elem : rhs) add_back(elem);
	}

  /** Destroy all items and reset the size to zero */
	void reset() { for (auto& item : *this) item.~T(); m_length = 0; m_start = 0; }

	~CircularBuffer<T, max_size>() {
		reset();
	}

	CircularBuffer& operator=(CircularBuffer& rhs) = delete;

	/**
	 * Get a raw pointer to the underlying array storage
	 * @warning This pointer may or may not be aligned!
	 */
	const array_t* get_raw() const { return m_array; }
  /** @see CircularBuffer::get_raw */
	array_t* get_raw() { return m_array; }

  /** Returns the number of elements currently in the buffer */
	size_t size() const { return m_length; }
  /** Returns the maximum number of elements this buffer can hold */
	size_t allocated() const { return max_size; }

  /** Returns true if size() == allocated() */
	bool full() const { return m_length == max_size; }
  /** Returns true if size() == 0 */
	bool empty() const { return m_length == 0; }

  /**
   * Returns a reference to the element at the start of
   * the buffer. If the buffer is empty, the behavior
   * of this function is undefined.
   */
	const T& front() const { return operator[](0); }
  /** @see CircularBufffer::front */
	T& front() { return operator[](0); }

  /**
   * Returns a reference to the element at the end of
   * the buffer. If the buffer is empty, the behavior
   * of this function is undefined.
   */
	const T& back() const { return operator[](m_length - 1); }
  /** @see CircularBuffer::back */
	T& back() { return operator[](m_length - 1); }

  /**
   * Array access of the buffer. This function performs no bounds checking.
   * @param index The element number to access, must be < size
   * @return A reference to the index element, or undefined if the index is invalid.
   */
	const T& operator[](const size_t index) const { return *reinterpret_cast<const T*>(&m_array[m_get_true_index(index, m_start)]); }
  /** @see CircularBuffer::operator[] */
	T& operator[](const size_t index) { return *reinterpret_cast<T*>(&m_array[m_get_true_index(index, m_start)]); }

	/**
	 * Remove the element on the end of the buffer.
	 * @return False if the buffer is empty, true otherwise.
	 */
	bool destroy_back() {
		if (m_length == 0) return false;
		// call the dtor of the item we're deleting
		(*this)[m_length - 1].~T();
		// decrement length
		m_length--;
		return true;
	}

  /**
   * Construct an element at the end of the buffer. This
   * function works similar to std::vector::emplace_back.
   * @param args Constructor arguments for T
   * @return false if the buffer is full, true otherwise.
   */
	template<typename ...Args>
	bool emplace_back(Args&& ... args) {
		// if the length is already maxed out, break
		if (m_length == max_size) {
			return false;
		}
		// construct the object in the correct place
		new(&operator[](m_length)) T(args...);
		// increment length
		m_length++;
		return true;
	}

  /**
   * Copy an element to the end of the buffer.
   * @param obj The object to copy.
   * @return False if the buffer is full, true otherwise.
   */
	bool add_back(const T& obj) {
		// if the length is already maxed out, break
		if (m_length == max_size) {
			return false;
		}
		// construct the object in the correct place
		new(&operator[](m_length)) T(obj);
		// increment length
		m_length++;
		return true;
	}

  /**
   * Remove the element at the start of the buffer.
   * @return False if the buffer is empty, true otherwise.
   */
	bool destroy_front() {
		if (m_length == 0) return false;
		// Destroy the object in the first position
		operator[](0).~T();
		// decrement length
		m_length--;
		// increment start
		if (m_start == max_size - 1) m_start = 0;
		else m_start++;
		return true;
	}

  /**
   * Construct an element at the start of the buffer. This
   * function works similar to std::vector::emplace.
   * @param args Constructor arguments for T
   * @return false if the buffer is full, true otherwise.
   */
	template<typename ...Args>
	bool emplace_front(Args&& ... args) {
		// if the length is already maxed out, break
		if (m_length == max_size) {
			return false;
		}
		// increment length
		m_length++;
		// decrement start
		if (m_start == 0) m_start = max_size - 1;
		else m_start--;
		// construct an object in the front spot
		new(&operator[](0)) T(args...);
		return true;
	}

  /**
   * Copy an element to the start of the buffer.
   * @param obj The object to copy.
   * @return False if the buffer is full, true otherwise.
   */
	bool add_front(const T& obj) {
		// if the length is already maxed out, break
		if (m_length == max_size) {
			return false;
		}
		// increment length
		m_length++;
		// decrement start
		if (m_start == 0) m_start = max_size - 1;
		else m_start--;
		// construct an object in the front spot
		new(&operator[](0)) T(obj);
		return true;
	}

	class Iterator {
		array_t* m_data;
		size_t m_position;
		size_t m_index;
	public:
		Iterator(array_t* _data, const size_t _position, const size_t index = 0)
			: m_data(_data)
			, m_position(_position)
			, m_index(index) {}

		T& operator*() { return *reinterpret_cast<T*>(&m_data[m_position]); }
		Iterator& operator++() { if (++m_position >= max_size) m_position = 0; m_index++; return *this; }
		bool operator!=(const Iterator& it) const { return m_index != it.m_index; }

		friend class CircularBuffer<T, max_size>;
	};

	class ConstIterator {
		const array_t* m_data;
		size_t m_position;
		size_t m_index;
	public:
		ConstIterator(const array_t* _data, const size_t _position, const size_t index = 0)
			: m_data(_data)
			, m_position(_position)
			, m_index(index) {}

		const T& operator*() const { return *reinterpret_cast<const T*>(&m_data[m_position]); }
		ConstIterator& operator++() { if (++m_position >= max_size) m_position = 0; m_index++; return *this; }
		bool operator!=(const ConstIterator& it) const { return m_index != it.m_index; }

		friend class CircularBuffer<T, max_size>;
	};

	Iterator begin() { return { m_array, m_start }; }
	const Iterator end() { return { m_array, 0, m_length }; }

	ConstIterator begin() const { return { m_array, m_start }; }
	const ConstIterator end() const { return { m_array, 0, m_length }; }

	const CircularBuffer<T, max_size>& crange() const noexcept { return *this; }

	/**
	 * Destroy an element at a given index. This function
	 * is based off of vector::remove, and requires an
	 * iterator as an index.
	 * @param iter The iterator representing the element to remove.
	 * This iterator is invalid after this function is called.
	 */
	void remove(const CircularBuffer<T, max_size>::ConstIterator& iter) {
		// destroy the element
		(*iter).~T();
		// copy the memory over from the back or the front, whichever is closest
		if (iter.m_index >= (m_length >> 1)) {
			// copy from the back
			for (auto i = iter.m_index; i < m_length - 1; i++) {
				m_array[m_get_true_index(i, m_start)] = m_array[m_get_true_index(i + 1, m_start)];
			}
		}
		else {
			// copy from the front
			for (auto i = iter.m_index; i > 0; i--) {
				m_array[m_get_true_index(i, m_start)] = m_array[m_get_true_index(i - 1, m_start)];
			}
			// inc start
			if (m_start == max_size - 1) m_start = 0;
			else m_start++;
		}
		// dec length
		m_length--;
	}

	/** @see CircularBuffer::remove */
	void remove(CircularBuffer<T, max_size>::Iterator& iter) {
		// destroy the element
		(*iter).~T();
		// copy the memory over from the back or the front, whichever is closest
		if (iter.m_index >= (m_length >> 1)) {
			// copy from the back
			for (auto i = iter.m_index; i < m_length - 1; i++) {
				m_array[m_get_true_index(i, m_start)] = m_array[m_get_true_index(i + 1, m_start)];
			}
		}
		else {
			// copy from the front
			for (auto i = iter.m_index; i > 0; i--) {
				m_array[m_get_true_index(i, m_start)] = m_array[m_get_true_index(i - 1, m_start)];
			}
			// inc start
			if (m_start == max_size - 1) m_start = 0;
			else m_start++;
		}
		// dec length
		m_length--;
	}

private:

	static size_t m_get_true_index(const size_t index, const size_t m_start) {
		const auto corIndex = m_start + index;
		return corIndex >= max_size ? corIndex - max_size : corIndex;
	}

	array_t m_array[max_size];
	size_t m_length;
	size_t m_start;
};