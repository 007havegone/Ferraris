#include "Renderer.h"
#include "GraphicsPlatformInterface.h"
#include "Direct3D12/D3D12Interface.h"

namespace ferraris::graphics {
namespace {
platform_interface gfx{};

bool
set_platform_interface(graphics_platform platform)
{
	switch (platform)
	{
	case ferraris::graphics::graphics_platform::direct3d12:
		d3d12::get_platform_interface(gfx);
		break;
	default:
		return false;
	}
	return true;
}

}// anonymous namespace

bool
initialize(graphics_platform platform)
{
	// Allocate the gfx interface according the Render API type
	// and call the initialize the Pipeline
	return set_platform_interface(platform) && gfx.initialize();
}

void
shutdown()
{
	gfx.shutdown();
}
void
render()
{
	gfx.render();
}
}