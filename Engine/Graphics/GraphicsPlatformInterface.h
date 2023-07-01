#pragma once
#include "CommonHeaders.h"
#include "Renderer.h"
#include "Platform\Window.h"
namespace ferraris::graphics{
// Current we only use the same initialization and shutdown	 for SDK
// If need more customize can use the class for inherited
struct platform_interface
{
	bool (*initialize)(void);
	void (*shutdown)(void);
	void (*render)(void);
	
	// anonymous struct
	struct {
		surface(*create)(platform::window);
		void (*remove)(surface_id);
		void (*resize)(surface_id, u32, u32);
		u32 (*width)(surface_id);
		u32 (*height)(surface_id);
		void (*render)(surface_id);
	} surface;
};
}