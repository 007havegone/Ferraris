#include "Renderer.h"
#include "GraphicsPlatformInterface.h"
#include "Direct3D12/D3D12Interface.h"

namespace ferraris::graphics {
namespace {
// Define the array of compiled engine shaders file is located for each one of supported APIs.
constexpr const char* engine_shader_paths[]{
	".\\shaders\\d3d12\\shaders.bin",
	// ".\\shaders\vulkan\\shaders.bin", etc.
};
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

	assert(gfx.platform == platform);
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

surface
create_surface(platform::window window)
{
	return gfx.surface.create(window);
}

void
remove_surface(surface_id id)
{
	assert(id::is_valid(id));
	gfx.surface.remove(id);
}

const char* 
get_engine_shaders_path()
{
	return engine_shader_paths[(u32)gfx.platform];
}

const char*
get_engine_shaders_path(graphics_platform platform)
{
	return engine_shader_paths[(u32)platform];
}
void
surface::resize(u32 width, u32 height) const
{
	assert(is_valid());
	gfx.surface.resize(_id, width, height);
}
u32
surface::width() const
{
	assert(is_valid());
	return gfx.surface.width(_id);
}

u32
surface::height() const
{
	assert(is_valid());
	return gfx.surface.height(_id);
}

void
surface::render() const
{
	assert(is_valid());
	gfx.surface.render(_id);

}
}