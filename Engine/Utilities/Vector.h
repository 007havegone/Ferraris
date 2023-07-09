#pragma once
#include "CommonHeaders.h"

namespace ferraris::utl {

/**
* A vector class similar to std::vector with basic fucntionality
* The user can specifiy in the template argument whether they want
* elements' destructor to be called when being removed or while
* clearing/destructing the vector
*/
template<typename T, bool destruct = true>
class vector
{
public:
	// Default constructor. Doesn't allocate memory.
	vector() = default;

	// Constructor resize the vector and initialization 'count' item
	constexpr explicit vector(u64 count)
	{
		resize(count);
	}

	// Constructor resizes the vector and initialized 'count' items using 'value'
	constexpr explicit vector(u64 count, const T& value)
	{
		resize(count, value);
	}

	template<typename it, typename = std::enable_if_t<std::_Is_iterator_v<it>>>
	constexpr explicit vector(it first, it last)
	{
		for (; first != last; ++first)
		{
			emplace_back(*first);
		}
	}


	// Copy-constructor. Constructs by copying another vector. The items
	// in the copied vector must be copyable.
	// Because we don't have manual copy the members.
	constexpr vector(const vector& o)
	{
		*this = o;
	}
	// Move-constructor. Constructs by moving another vector.
	// The original vector will be empty after move.
	constexpr vector(vector&& o)
		: _capacity{ o._capacity }, _size{ o._size }, _data{ o._data }
	{
		o.reset();
	}

	// Copy-assignment operator. Clears this vector and copied items
	// from another one vector. The items must be copyable.
	constexpr vector& operator=(const vector& o)
	{
		assert(this != std::addressof(o)); // here does not support self assignment.
		if (this != std::addressof(o))
		{
			clear();
			reserve(o._size);
			for (auto& item : o)
			{
				emplace_back(item);
			}
			assert(_size == o._size);
		}
		return *this;
	}
	// Move-assignment operator. Frees all resources in this vector and
	// moves the other vector into this one.
	constexpr vector& operator=(vector&& o)
	{
		assert(this != std::addressof(o)); // here does not support self assignment.
		if (this != std::addressof(o))
		{
			destroy();
			move(o);
		}
		return *this;
	}


	// Destructs the vector and its items as specified in template argument.
	~vector() { destroy(); }


	// Insert an item at the end of the vector by copying 'value'
	constexpr void push_back(const T& value)
	{
		emplace_back(value);
	}
	// Insert an item at the end of the vector by moving 'value'
	constexpr void push_back(T&& value)
	{
		emplace_back(std::move(value));
	}

	// C++ 17 emplace_back like, return the item value
	// Copy- or Move-constructs an item at the end of the vector.
	template<class... params>
	constexpr decltype(auto) emplace_back(params&&... p)
	{
		if (_size == _capacity)
		{
			reserve(((_capacity + 1) * 3) >> 1); // reserve 50% more
		}
		T* const item{ new (std::addressof(_data[_size])) T(std::forward<params>(p)...) };
		++_size;
		return *item;
	}

	// Resizes the vector and initializes new items with their default value.
	constexpr void resize(u64 new_size)
	{
		// C++17 feature template variable, equal to std::is_default_constructible<T>::value
		static_assert(std::is_default_constructible_v<T>,
			"Type must be default-constructible.");

		if (new_size > _size)
		{
			reserve(new_size);
			while (_size < new_size)
			{
				emplace_back();
			}
		}
		else if (new_size < _size)
		{
			if constexpr (destruct)
			{
				destruct_range(new_size, _size);
			}
			_size = new_size;
		}
		// Do nothing is new_size == _size
		assert(new_size == _size);
	}

	// Resizes the vector and initializes new items by copying 'value'.
	constexpr void resize(u64 new_size, const T& value)
	{
		static_assert(std::is_copy_constructible_v<T>,
			"Type must be copy_constructible.");

		if (new_size > _size)
		{
			reserve(new_size);
			while (_size < new_size)
			{
				emplace_back(value);
			}
		}
		else if (new_size < _size)
		{
			if constexpr (destruct)
			{
				destruct_range(new_size, _size);
			}
			_size = new_size;
		}
		// Do nothing is new_size == _size
		assert(new_size == _size);
	}

	// Allocates memory to contain the specified number of items.
	constexpr void reserve(u64 new_capacity)
	{
		if (new_capacity > _capacity)
		{
			// NOTE: realloc() will automatically copy the data in the buffer
			//		 if a new region of memory is allocated.
			void* new_buffer{ realloc(_data, new_capacity * sizeof(T)) };
			assert(new_buffer);
			if (new_buffer)
			{
				_data = static_cast<T*>(new_buffer);
				_capacity = new_capacity;
			}
		}
	}

	// Remove the item at specified index
	constexpr T* const erase(u64 index)
	{
		assert(_data && index < _size);
		return erase(std::addressof(_data[index]));
	}

	// Remove the item at specified location
	constexpr T* const earse(T* const item)
	{
		assert(_data && item >= std::addressof(_data[0]) &&
			item < std::addressof(_data[_size]));

		if constexpr (destruct) item->T();
		--size;
		// Only move if not the last one item
		if (item < std::addressof(_data[_size]))
		{
			memcpy(item, item + 1, (std::addressof(_data[_size]) - item) * sizeof(T));
		}
		return item;
	}

	// Same as erase but faster because it juest copied the last item
	constexpr T* const erase_unordered(u64 index)
	{
		assert(_data && index < _size);
		return earse_unordered(std::addressof(_data[index]));
	}

	// Same as erase but faster because it juest copied the last item
	constexpr T* const earse_unordered(T* const item)
	{
		assert(_data && item >= std::addressof(_data[0]) &&
			item < std::addressof(_data[_size]));
		if constexpr (destruct) item->~T();
		--_size;

		if (item < std::addressof(_data[_size]))
		{
			memcpy(item, std::addressof(_data[_size]), sizeof(T));
		}
		return item;
	}

	// Clears the vector and destructs as specified in template argument.
	constexpr void clear()
	{
		if constexpr (destruct)
		{
			destruct_range(0, _size);
		}
		_size = 0;
	}

	constexpr void swap(vector& o)
	{
		if (this != std::addressof(o))
		{
			auto temp{ o };
			o = *this;
			*this = temp;
		}
	}

	// Pointer to the start of data. Might be nullptr.
	[[nodiscard]] constexpr T* data()
	{
		return _data;
	}
	// Pointer to the start of data. Might be nullptr.
	[[nodiscard]] constexpr const T* data() const
	{
		return _data;
	}

	// Return true if the vector is empty.
	[[nodiscard]] constexpr bool empty() const
	{
		return _size == 0;
	}

	// Return the number of items of the vector.
	[[nodiscard]] constexpr u64 size() const
	{
		return _size;
	}
	// Return the capacity of the vector
	[[nodiscard]] constexpr u64 capacity() const
	{
		return _capacity;
	}
	// Indexing operator. Return a reference to the item at specified index.
	[[nodiscard]] T& operator[](u64 index)
	{
		assert(_data && index < _size);
		return _data[index];
	}

	// Indexing operator. Return a const reference to the item at specified index.
	[[nodiscard]] const T& operator[](u64 index) const
	{
		assert(_data && index < _size);
		return _data[index];
	}

	// Return a reference to the first item.
	// Wiil fault the application if called when the vector is empty.
	[[nodiscard]] T& front()
	{
		assert(_data && _size);
		return _data[0];
	}

	// Return a const reference to the first item.
	// Wiil fault the application if called when the vector is empty.
	[[nodiscard]] const T& front() const
	{
		assert(_data && _size);
		return _data[0];
	}

	// Return a const reference to the last item.
	// Wiil fault the application if called when the vector is empty.
	[[nodiscard]] T& back()
	{
		assert(_data && _size);
		return _data[_size - 1];
	}

	// Return a const reference to the last item.
	// Wiil fault the application if called when the vector is empty.
	[[nodiscard]] const T& back() const
	{
		assert(_data && _size);
		return _data[_size - 1];
	}

	// Return the pointer to the first item.
	// Return nullptr when vector is empty.
	[[nodiscard]] T* begin()
	{
		assert(_data);
		return std::addressof(_data[0]);
	}

	// Return the const pointer to the first item.
	// Return nullptr when vector is empty.
	[[nodiscard]] const T* begin() const
	{
		assert(_data);
		return std::addressof(_data[0]);
	}

	// Return the pointer to the last item.
	// Return nullptr when vector is empty.
	[[nodiscard]] T* end()
	{
		assert(_data);
		return std::addressof(_data[_size]);
	}

	// Return the const pointer to the last item.
	// Return nullptr when vector is empty.
	[[nodiscard]] const T* end() const
	{
		assert(_data);
		return std::addressof(_data[_size]);
	}

private:
	constexpr void move(vector& o)
	{
		_capacity = o._capacity;
		_size = o._size;
		_data = o._data;
		o.reset();
	}

	constexpr void reset()
	{
		_capacity = 0;
		_size = 0;
		_data = nullptr;
	}

	constexpr void destruct_range(u64 first, u64 last)
	{
		assert(destruct);
		assert(first <= last && last <= _size);
		if (_data)
		{
			for (; first != last; ++first)
			{
				_data[first].~T();
			}
		}
	}

	constexpr void destroy()
	{
		assert([&] { return _capacity ? _data != nullptr : _data == nullptr; }());
		clear();
		_capacity = 0;
		if (_data) free(_data);
		_data = nullptr;
	}
	u64 _capacity{ 0 };
	u64 _size{ 0 };
	T* _data{ nullptr };

};
}