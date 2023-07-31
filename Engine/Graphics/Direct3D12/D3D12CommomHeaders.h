#pragma once
#include "CommonHeaders.h"
#include "Graphics/Renderer.h"

// Skip definition of min/max macros in window.h
#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>


#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace ferraris::graphics::d3d12{
constexpr u32 frame_buffer_count{ 3 };
using id3d12_device = ID3D12Device8;
using id3d12_graphics_command_list = ID3D12GraphicsCommandList6;
}

// Assert that COM call to D3D API successed

#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)									\
if(FAILED(x)) {										\
	char line_number[32];							\
	sprintf_s(line_number, "%u", __LINE__);			\
	OutputDebugStringA("%Error in:");				\
	OutputDebugStringA(__FILE__);					\
	OutputDebugStringA("\nLine: ");					\
	OutputDebugStringA(line_number);				\
	OutputDebugStringA("\n");						\
	OutputDebugStringA(#x);							\
	OutputDebugStringA("\n");						\
	__debugbreak();									\
}													
#endif // !DXCall
#else
#ifndef DXCall
#define DXCall(x) x
#endif // !DXCall
#endif // _DEBUG

#ifdef _DEBUG
// Set the name of the COM object and output a debug string int Visual's studio output pannel.
#define NAME_D3D12_OBJECT(obj, name) obj->SetName(name); OutputDebugString(L"::D3D12 Object Created: "); OutputDebugString(name); OutputDebugString(L"\n");
// The index variant will include the index in the name of the object
#define NAME_D3D12_OBJECT_INDEX(obj, n, name)		\
{													\
wchar_t fullname[128];								\
if(swprintf_s(fullname, L"%s[%u]", name, n) > 0){	\
	obj->SetName(fullname);							\
	OutputDebugString(L"::D3D12 Object Created: ");	\
	OutputDebugString(fullname);					\
	OutputDebugString(L"\n");						\
}}
#else
#define NAME_D3D12_OBJECT(x, name)
#define NAME_D3D12_OBJECT_INDEX(x, n, name)
#endif

// These two headers are included almost all modules
#include "D3D12Helpers.h"
#include "D3D12Resources.h"