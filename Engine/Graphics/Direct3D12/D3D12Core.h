#pragma once
#include "D3D12CommomHeader.h"

namespace ferraris::graphics::d3d12::core {

bool initialize();
void shutdown();
void render();
/// <summary>
/// The general function for release the source in Direct 3D like Command Queue, List and adapter.
/// </summary>
/// <param name="resource"> The resource will release</param>
template<typename T>
constexpr void release(T*& resource)
{
	if (resource)
	{
		resource->Release();
		resource = nullptr;
	}
}

ID3D12Device* const device();

}
