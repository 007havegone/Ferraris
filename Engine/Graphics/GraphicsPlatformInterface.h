#pragma once
#include "CommonHeaders.h"
#include "Renderer.h"

namespace ferraris::graphics{
// Current we only use the same initialization and shutdown	 for SDK
// If need more customize can use the class for inherited
struct platform_interface
{
	bool (*initialize)(void);
	void (*shutdown)(void);
	void (*render)(void);
};
}