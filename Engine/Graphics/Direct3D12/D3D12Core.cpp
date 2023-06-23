#include "D3D12Core.h"

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using namespace Microsoft::WRL;

namespace ferraris::graphics::d3d12::core {
namespace {

	/// <summary>
	/// 
	/// </summary>
	class d3d12_command
{
	explicit d3d12_command(ID3D12Device8* const device, D3D12_COMMAND_LIST_TYPE type) 
	{
		HRESULT hr{ S_OK };
		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;// config for queue execute on specific GPU
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Type = type;
		DXCall(hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_cmd_queue))); 
		if (FAILED(hr)) goto _error;
		NAME_D3D12_OBJECT(_cmd_queue,
						  type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
						  L"GFX Command Queue" :
						  type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
						  L"Compute Command Queue" : L"Command Queue");

		for (u32 i{ 0 }; i < frame_buffer_count; ++i)
		{
			command_frame& frame{ _cmd_frames[i] };
			DXCall( hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.cmd_allocator)));
			if (FAILED(hr)) goto _error;
			NAME_D3D12_OBJECT_INDEX(frame.cmd_allocator, i,
									type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
									L"GFX Command Allocator" :
									type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
									L"Compute Command Allocator" : L"Command Allocator");

		}
		DXCall(hr = device->CreateCommandList(0, type, _cmd_frames[0].cmd_allocator , nullptr, IID_PPV_ARGS(&_cmd_list)));
		DXCall(_cmd_list->Close());// Reset the command list
		NAME_D3D12_OBJECT(_cmd_queue,
						  type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
						  L"GFX Command List" :
						  type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
						  L"Compute Command List" : L"Command List");

		_error:
			release();
	}
	// Resetting the command allocator will free memory used by previously record commands.
	// Resetting the command list will reopen it for recording new commands.
	void begin_frame()
	{
		command_frame& frame{ _cmd_frames[_frame_index] };
		frame.wait();
		DXCall(frame.cmd_allocator->Reset());
		DXCall(_cmd_list->Reset(frame.cmd_allocator, nullptr));// the pipeline state object describe the GPU with shaders and resources should be used, and more
	}
	void end_frame()
	{
		DXCall(_cmd_list->Close());
		ID3D12CommandList* const cmd_lists[]{ _cmd_list };
		_cmd_queue->ExecuteCommandLists(_countof(cmd_lists), &cmd_lists[0]);
		_frame_index = (_frame_index + 1) % frame_buffer_count;
	}
	void release()
	{

	}
private:
	struct command_frame
	{
		ID3D12CommandAllocator* cmd_allocator{ nullptr };

		void release()
		{
			core::release(cmd_allocator);
		}
		void wait()
		{

		}
	};
	ID3D12CommandQueue*			_cmd_queue{ nullptr };
	ID3D12GraphicsCommandList6* _cmd_list{ nullptr };
	command_frame				_cmd_frames[frame_buffer_count]{};
	u32							_frame_index{ 0 };


};

ID3D12Device8* main_device{ nullptr };
IDXGIFactory7* dxgi_factory{ nullptr };

constexpr D3D_FEATURE_LEVEL minimum_feature_level{ D3D_FEATURE_LEVEL_11_0 };


bool
failed_init()
{
	shutdown();
	return false;
}

// get the first most performing adapter that supports the minimum feature level.
// NOTE: This function can be expanded in functionality with, for example, checking if any
//		output devices (i.e. screens) are attached, enumerate the support resolution, provide 
//		a means for the user to choose which adapter to use in a multi-adapter setting, etc.
IDXGIAdapter4*
determine_main_adapter()
{
	IDXGIAdapter4* adapter{ nullptr };

	// get adapter in descending order of the performance
	for (u32 i{ 0 };
		dxgi_factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		// pick the first adapter that support the minimum feature level.
		if (SUCCEEDED(D3D12CreateDevice(adapter, minimum_feature_level, __uuidof(ID3D12Device), nullptr)))
		{
			return adapter;
		}
		release(adapter);
	}
	return nullptr;
}
D3D_FEATURE_LEVEL
get_max_feature_level(IDXGIAdapter4* adapter)
{
	constexpr D3D_FEATURE_LEVEL feature_levels[4]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
	};
	D3D12_FEATURE_DATA_FEATURE_LEVELS feature_level_info{};
	feature_level_info.NumFeatureLevels = _countof(feature_levels);
	feature_level_info.pFeatureLevelsRequested = feature_levels;

	ComPtr<ID3D12Device> device;
	DXCall(D3D12CreateDevice(adapter, minimum_feature_level, IID_PPV_ARGS(&device)));
	DXCall(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_level_info, sizeof(feature_level_info)));
	return feature_level_info.MaxSupportedFeatureLevel;
}
} // anonymous namespace

bool
initialize()
{
	// determine which adapter (i.e. graphics card) to use
	// determine what is the maximun feature level that is support
	// create a ID3D12Device (this is virtual adapter).
	if (main_device) shutdown();

	u32 dxgi_factory_flags{ 0 };
#ifdef _DEBUG
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	// Using the bracket, the ComPtr will auto release
	{
		ComPtr<ID3D12Debug3> debug_interface;
		DXCall(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
		debug_interface->EnableDebugLayer();
		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	}

#endif // _DEBUG

	HRESULT hr{ S_OK };
	DXCall( hr = CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));
	//CreateDXGIFactory2(dxgi_factory_flags, __uuidof(IDXGIFactory7), (void**)dxgi_factory);
	if (FAILED(hr)) return failed_init();


	// determine which adapter (i.e. graphics card£©to use, if any
	ComPtr<IDXGIAdapter4> main_adapter;
	main_adapter.Attach(determine_main_adapter());
	if (!main_adapter) return failed_init();

	D3D_FEATURE_LEVEL max_feature_level{ get_max_feature_level(main_adapter.Get()) };
	assert(max_feature_level >= minimum_feature_level);
	if (max_feature_level < minimum_feature_level) return failed_init();

	DXCall(hr = D3D12CreateDevice(main_adapter.Get(), max_feature_level, IID_PPV_ARGS(&main_device)));
	if (FAILED(hr)) return failed_init();

	NAME_D3D12_OBJECT(main_device, L"Main D3D12 DEVICE");

#ifdef _DEBUG
	{
		ComPtr<ID3D12InfoQueue> info_queue;
		DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));

		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}
#endif
	return true;
}

void
shutdown()
{
	release(dxgi_factory);
#ifdef _DEBUG
	{
		{
			ComPtr<ID3D12InfoQueue> info_queue;
			DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
		}
		ComPtr<ID3D12DebugDevice2> debug_device;
		DXCall(main_device->QueryInterface(IID_PPV_ARGS(&debug_device)));
		release(main_device);
		DXCall(debug_device->ReportLiveDeviceObjects(
			D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));

	}
#endif // _DEBUG

	release(main_device);
}

void begin_frame()
{

}
void end_frame()
{

}
void
render()
{
	begin_frame();
	end_frame();
}
}

