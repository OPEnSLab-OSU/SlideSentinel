#pragma once
#include <stdlib.h>

/**
 * Templated implementation of a fixed-size deque
 * To be used as a stack in Loom Network
 *
 */

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

	void reset() { for (auto& item : *this) item.~T(); m_length = 0; m_start = 0; }

	~CircularBuffer<T, max_size>() {
		reset();
	}

	CircularBuffer& operator=(CircularBuffer& rhs) = delete;

	/** misc functions */
	const array_t* get_raw() const { return m_array; }
	array_t* get_raw() { return m_array; }

	size_t size() const { return m_length; }
	size_t allocated() const { return max_size; }

	bool full() const { return m_length == max_size; }
	bool empty() const { return m_length == 0; }

	const T& front() const { return operator[](0); }
	T& front() { return operator[](0); }

	const T& back() const { return operator[](m_length - 1); }
	T& back() { return operator[](m_length - 1); }

	/** manipulation */

	const T& operator[](const size_t index) const { return *reinterpret_cast<const T*>(&m_array[m_get_true_index(index, m_start)]); }
	T& operator[](const size_t index) { return *reinterpret_cast<T*>(&m_array[m_get_true_index(index, m_start)]); }

	/** pop/push */
	bool destroy_back() {
		if (m_length == 0) return false;
		// call the dtor of the item we're deleting
		(*this)[m_length - 1].~T();
		// decrement length
		m_length--;
		return true;
	}

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

	/** Iterators */
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

	/** destroy at index */
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

	/** destroy at index */
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