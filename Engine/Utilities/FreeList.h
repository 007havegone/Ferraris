#pragma once
#include "CommonHeaders.h"

namespace ferraris::utl {

#if USE_STL_VECTOR
#pragma message("WARNING: using utl::free_list with std::vector result in duplicate calls to class constructor!")
#endif
/**
* In free list, the we use the first 4 bytes of element as
* the free idnex.
*/
template<typename T>
class free_list
{
	static_assert(sizeof(T) >= sizeof(u32));
public:
	free_list() = default;
	explicit free_list(u32 count)
	{
		_array.reserve(count);
	}
	~free_list()
	{
		assert(!_size);
		// Before calling the destructor of utl::vector.
		// set the memory to 0, which may contain the valid data.
		// As for window_info is ok.
#if USE_STL_VECTOR
		memset(_array.data(), 0, _array.size() * sizeof(T));
#endif
	}
	// add a new item and return its index
	template<class... params>
	constexpr u32 add(params&&... p)
	{
		u32 id{ u32_invalid_id };
		if (_next_free_index == u32_invalid_id)
		{
			id = (u32)_array.size();
			_array.emplace_back(std::forward<params>(p)...);
		}
		else
		{
			id = _next_free_index;
			assert(id < _array.size() && already_removed(id));
			_next_free_index = *(const u32 *const)std::addressof(_array[id]);// update the head
			new (std::addressof(_array[id])) T(std::forward<params>(p)...);// add the item in slot
		}
		++_size;
		return id;
	}
	constexpr void remove(u32 id)
	{
		assert(id < _array.size() && !already_removed(id));
		T& item{ _array[id] };
		item.~T();
		DEBUG_OP(memset(std::addressof(_array[id]), 0xcc, sizeof(T)));
		*(u32 * const)std::addressof(_array[id]) = _next_free_index;// slot point to head
		_next_free_index = id;// update the head
		--_size;
	}

	constexpr u32 size() const
	{
		return _size;
	}

	constexpr u32 capacity() const
	{
		return _array.size();
	}

	constexpr bool empty() const
	{
		return _size == 0;
	}

	[[nodiscard]] constexpr T& operator[](u32 id)
	{
		assert(id < _array.size() && !already_removed(id));
		return _array[id];
	}

	[[nodiscard]] constexpr const T& operator[](u32 id) const
	{
		assert(id < _array.size() && !already_removed(id));
		return _array[id];
	}

private:
	constexpr bool already_removed(u32 id)
	{
		// NOTE: when sizeof(T) == sizeof(u32)
		// we can't test if the item was already removed.
		if constexpr (sizeof(T) > sizeof(u32))
		{
			u32 i{ sizeof(u32) }; // skip the first 4 bytes
			const u8* const p{ (const u8* const)std::addressof(_array[id]) };
			while( p[i] == 0xcc && i < sizeof(T) ) i++;
			return i == sizeof(T);
		}
		// 4 bytes not enough, check by otherwise
		else
		{
			return true;
		}
	}
#if USE_STL_VECTOR
	utl::vector<T>		_array;
#else
	utl::vector<T, false>		_array;
#endif
	u32							_next_free_index{ u32_invalid_id }; // free index head
	u32							_size{ 0 }; // size of element
};
}