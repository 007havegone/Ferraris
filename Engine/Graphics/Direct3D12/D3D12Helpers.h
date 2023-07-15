#pragma once
#include "D3D12CommomHeaders.h"

namespace ferraris::graphics::d3d12::d3dx{

// ref to DirectX-Graphics-Samples d3dx12.h
constexpr struct {
	D3D12_HEAP_PROPERTIES default_heap
	{
		D3D12_HEAP_TYPE_DEFAULT,				// Type
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,		// CPUPageProperty
		D3D12_MEMORY_POOL_UNKNOWN,				// MemoryPoolPreference
		0,										// CreationNodeMask
		0										// VisibleNodeMask
	};
} heap_properties;
}
