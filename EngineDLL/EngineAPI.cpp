#include "Common.h"
#include "CommonHeaders.h"
#include "..\Engine\Components\Script.h"
#include "..\Graphics\Renderer.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"

/* following define remove the useless define include into this file,
 * increase the compile speed.
 */ 
#ifndef WIN32_LEAN_MEAN_AND
#define WIN32_LEAN_MEAN_AND
#endif

#include <Windows.h> // load dll by win32 function
#include <atlsafe.h>
using namespace ferraris;

namespace {
HMODULE game_code_dll{ nullptr };
using _get_script_creator = ferraris::script::detail::script_creator(*)(size_t);
_get_script_creator get_script_creator{ nullptr };
using _get_script_names = LPSAFEARRAY(*)(void);
_get_script_names get_script_names{ nullptr };

utl::vector<graphics::render_surface> surfaces;
} // anonymous namespace

EDITOR_INTERFACE u32
LoadGameCodeDll(const char* dll_path)
{
	if (game_code_dll) return FALSE; // have loaded
	game_code_dll = LoadLibraryA(dll_path);
	assert(game_code_dll);

	get_script_creator = (_get_script_creator)GetProcAddress(game_code_dll, "get_script_creator");
	get_script_names = (_get_script_names)GetProcAddress(game_code_dll, "get_script_names");

	return (game_code_dll && get_script_creator && get_script_names) ? TRUE : FALSE;// if load success
}

EDITOR_INTERFACE u32
UnloadGameCodeDll()
{
	if (!game_code_dll) return FALSE;
	assert(game_code_dll);
	[[maybe_unused]] int result{ FreeLibrary(game_code_dll) };
	assert(result);
	game_code_dll = nullptr;
	return TRUE;
}

EDITOR_INTERFACE script::detail::script_creator
GetScriptCreator(const char* name)
{
	return (game_code_dll && get_script_creator) ? get_script_creator(script::detail::string_hash()(name)) : nullptr;
}

EDITOR_INTERFACE LPSAFEARRAY
GetScriptNames()
{
	return (game_code_dll && get_script_names) ? get_script_names() : nullptr;
}

EDITOR_INTERFACE u32
CreateRenderSurface(HWND host, s32 width, s32 height)
{
	assert(host);
	platform::window_init_info info{ nullptr, host, nullptr, 0,0, width, height };
	graphics::render_surface surface{ platform::create_window(&info), {} };
	assert(surface.window.is_valid());
	surfaces.emplace_back(surface);
	return (u32)surfaces.size() - 1;
}

EDITOR_INTERFACE void
RemoveRenderSurface(u32 id)
{
	assert(id < surfaces.size());
	platform::remove_window(surfaces[id].window.get_id());
	// here do not need to remove from surfaces, we put it into the free-list container.
}

EDITOR_INTERFACE HWND
GetWindowHandle(u32 id)
{
	assert(id < surfaces.size());
	return (HWND)surfaces[id].window.handle();
}

EDITOR_INTERFACE void
ResizeRenderSurface(u32 id)
{
	assert(id < surfaces.size());
	surfaces[id].window.resize(0, 0);
}