#include "D3D12Resources.h"
#include "D3D12Core.h"
#include "D3D12Helpers.h"
namespace ferraris::graphics::d3d12 {


#pragma region DESCRIPTOR_HEAP
bool
descriptor_heap::initialize(u32 capacity, bool is_shader_visible)
{
	std::lock_guard lock{ _mutex };
	assert(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
	assert(!(_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER &&
			capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));

	if (_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV ||
		_type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
	{
		is_shader_visible = false;
	}
	release();
	ID3D12Device* const device{ core::device() };
	assert(device);

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags = is_shader_visible
		? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		: D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	desc.NumDescriptors = capacity;
	desc.Type = _type;

	HRESULT hr{ S_OK };
	DXCall(hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap)));
	if (FAILED(hr)) return false;

	_free_handles = std::move(std::make_unique<u32[]>)(capacity);
	_capacity = capacity;
	_size = 0;
	for (u32 i{ 0 }; i < capacity; ++i) _free_handles[i] = i;
	DEBUG_OP(for (u32 i{ 0 }; i < frame_buffer_count; ++i) assert(_deferred_free_indices[i].empty()));

	_descriptor_size = device->GetDescriptorHandleIncrementSize(_type);
	_cpu_start = _heap->GetCPUDescriptorHandleForHeapStart();
	// only if the desciptor is shader_visible, we also get the GPU Address.
	_gpu_start = is_shader_visible ?
		_heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

	return true;
}

void
descriptor_heap::release()
{
	assert(!_size);
	core::deferred_release(_heap);
}

void
descriptor_heap::process_deferred_free(u32 frame_idx)
{
	std::lock_guard lock{ _mutex };
	assert(frame_idx < frame_buffer_count);

	// get the index vector for frame
	utl::vector<u32>& indices{ _deferred_free_indices[frame_idx] };
	if (!indices.empty())
	{
		for (auto index : indices)// add the index to the free handles
		{
			--_size;
			_free_handles[_size] = index;
		}
		indices.clear();
	}
}

descriptor_handle
descriptor_heap::allocate()
{
	std::lock_guard lock{ _mutex };
	assert(_heap);
	assert(_size < _capacity);
	// calculate the next free address
	const u32 index{ _free_handles[_size] };
	const u32 offset{ index * _descriptor_size };
	++_size;

	descriptor_handle handle;
	handle.cpu.ptr = _cpu_start.ptr + offset;
	if (is_shader_visible())
	{
		handle.gpu.ptr = _gpu_start.ptr + offset;
	}
#ifdef _DEBUG
	// because of the friend class
	DEBUG_OP(handle.container = this);
	DEBUG_OP(handle.index = index);
#endif
	return handle;

}


void
descriptor_heap::free(descriptor_handle& handle)
{
	if (!handle.is_valid()) return;

	std::lock_guard lock{ _mutex };
	assert(_heap && _size);
	assert(handle.container == this);
	assert(handle.cpu.ptr >= _cpu_start.ptr);
	assert((handle.cpu.ptr - _cpu_start.ptr) % _descriptor_size == 0);
	assert(handle.index < _capacity);
	const u32 index{ (u32)(handle.cpu.ptr - _cpu_start.ptr) / _descriptor_size };
	assert(handle.index == index);
	
	// Removing the resource only if the resource is useless.
	const u32 frame_idx{ core::current_frame_index() };
	_deferred_free_indices[frame_idx].push_back(index);
	core::set_deferred_release_flag();
	handle = {};
}
#pragma endregion


#pragma region TEXTURE

// Using the resource pointer to create the texture
d3d12_texture::d3d12_texture(d3d12_texture_init_info info)
{
	auto* const device{ core::device() };
	assert(device);

	D3D12_CLEAR_VALUE * const clear_value
	{
		(info.desc &&
		(info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
		 info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
		? &info.clear_value : nullptr
	};


	// If we have a resource pointer just giving.
	if (info.resource)
	{
		_resource = info.resource;
	}
	// Otherwise, create it
	else if (info.heap && info.desc)
	{
		assert(!info.resource);
		DXCall(device->CreatePlacedResource(info.heap, info.allocation_info.Offset, info.desc,
			info.initial_state, clear_value, IID_PPV_ARGS(&_resource)));
	}
	else if(info.desc)
	{
		// ID3D12Device::CreateCommittedResource, create both resource and an implicit heap, resource is  mapped to heap
		// ID3D12Device::CreatePlaceResource, create a resource placed in a specific heap. The lightest weight resource object, fastest to create and destroy
		// ID3D12Device::CreateReservedResource, create a resource that is reserved, and not yes mapped to any pages in a heap.
		// The third case mainly used when dealing with resource that are beging streamed into gpu memory.
		assert(!info.resource);
		// just setting the type of heap
		DXCall(device->CreateCommittedResource(&d3dx::heap_properties.default_heap, D3D12_HEAP_FLAG_NONE, info.desc,
			info.initial_state, clear_value, IID_PPV_ARGS(&_resource)));

	}
	assert(_resource);
	_srv = core::srv_heap().allocate();
	device->CreateShaderResourceView(_resource, info.srv_desc, _srv.cpu);


}

void
d3d12_texture::release()
{
	core::srv_heap().free(_srv);
	core::deferred_release(_resource);
}
#pragma endregion

}