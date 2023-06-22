#pragma once
#include "D3D12CommomHeader.h"

namespace ferraris::graphics::d3d12::core {

bool initialize();
void shutdown();

template<typename T>
constexpr void release(T*& resource)
{
	if (resource)
	{
		resource->Release();
		resource = nullptr;
	}
}

}
