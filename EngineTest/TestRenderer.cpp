#ifdef TEST_RENDERER
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"
#include "TestRenderer.h"


using namespace ferraris;
// here we use the render_surface to abstract different Render API
graphics::render_surface _surfaces[4];

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		bool all_closed{ true };
		for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
		{
			if (!_surfaces[i].window.is_closed())
			{
				all_closed = false;
			}
		}
		if (all_closed)
		{
			PostQuitMessage(0);
			return 0;
		}
	}
	break;
	case WM_SYSCHAR:
		if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN)) // alt + enter is down
		{
			platform::window win{ platform::window_id{(id::id_type)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };
			win.set_fullscreen(!win.is_fullscreen());
			return 0;
		}
		break;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
void
create_render_surface(graphics::render_surface& surface, platform::window_init_info info)
{
	surface.window = platform::create_window(&info);
}

bool
engine_test::initialize()
{
	bool result{ graphics::initialize(graphics::graphics_platform::direct3d12) };
	if (!realloc) return result;
	platform::window_init_info info[]
	{
		{&win_proc, nullptr, L"Test window 1", 100 - 2000, 100, 400, 800},
		{&win_proc, nullptr, L"Test window 2", 150 - 2000, 150, 800, 400},
		{&win_proc, nullptr, L"Test window 3", 200 - 2000, 200, 400, 400},
		{&win_proc, nullptr, L"Test window 4", 250 - 2000, 250, 800, 600},
	};
	static_assert(_countof(info) == _countof(_surfaces));

	for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
		create_render_surface(_surfaces[i], info[i]);
	return true;

}

void
engine_test::run()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

}
void
destroy_render_surface(const graphics::render_surface& surface)
{
	platform::remove_window(surface.window.get_id());
}

void
engine_test::shutdown()
{
	for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
		destroy_render_surface(_surfaces[i]);
	graphics::shutdown();
}

#endif // TEST_RENDERER