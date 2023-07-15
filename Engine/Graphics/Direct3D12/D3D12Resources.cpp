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
	auto* const device{ core::device() };
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

#pragma region RENDER_TEXTURE
d3d12_render_texture::d3d12_render_texture(d3d12_texture_init_info info)
	: _texture{ info }
{
	_mip_count = resource()->GetDesc().MipLevels;
	assert(_mip_count && _mip_count < d3d12_texture::max_mips);

	descriptor_heap& rtv_heap{ core::rtv_heap() };
	D3D12_RENDER_TARGET_VIEW_DESC desc{};
	desc.Format = info.desc->Format;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	auto * const device{ core::device() };
	assert(device);

	for (u32 i{ 0 }; i < _mip_count; ++i)
	{
		_rtv[i] = rtv_heap.allocate();
		device->CreateRenderTargetView(resource(), &desc, _rtv[i].cpu);
		++desc.Texture2D.MipSlice;
	}
}
void
d3d12_render_texture::release()
{
	for (u32 i{ 0 }; i < _mip_count; ++i) core::rtv_heap().free(_rtv[i]);
	_texture.release();
	_mip_count = 0;
}
#pragma endregion


#pragma region DEPTH_BUFFER
d3d12_depth_buffer::d3d12_depth_buffer(d3d12_texture_init_info info)
{
	// The SRV is diff from the DSV format.
	// We can't directly use the DSV format to create the SRV (_texture member)
	assert(info.desc);
	const DXGI_FORMAT dsv_format{ info.desc->Format };

	// Create to srv_desc by dsv__desv
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
	if (info.desc->Format == DXGI_FORMAT_D32_FLOAT)
	{
		info.desc->Format = DXGI_FORMAT_R32_TYPELESS;
		srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
	}

	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.PlaneSlice = 0;
	srv_desc.Texture2D.ResourceMinLODClamp = 0.f;
	// Create the texture with typeless format
	assert(!info.srv_desc && !info.resource);
	info.srv_desc = &srv_desc;
	_texture = d3d12_texture(info);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
	dsv_desc.Format = dsv_format;
	dsv_desc.Texture2D.MipSlice = 0;

	_dsv = core::dsv_heap().allocate();

	auto* const device{ core::device() };
	assert(device);
	device->CreateDepthStencilView(resource(), &dsv_desc, _dsv.cpu);

}
void d3d12_depth_buffer::release()
{
	core::dsv_heap().free(_dsv);
	_texture.release();
}
#pragma endregion

}