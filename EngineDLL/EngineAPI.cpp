#include "Common.h"
#include "CommonHeaders.h"

/* following define remove the useless define include into this file,
 * increase the compile speed.
 */ 
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#endif

#include <Windows.h> // load dll by win32 function

using namespace ferraris;

namespace {
	HMODULE game_code_dll{ nullptr };
} // anonymous namespace

EDITOR_INTERFACE u32
LoadGameCodeDll(const char* dll_path)
{
	if (game_code_dll) return FALSE; // have loaded
	game_code_dll = LoadLibraryA(dll_path);
	assert(game_code_dll);

	return game_code_dll ? TRUE : FALSE;// if load success
}

EDITOR_INTERFACE u32
UnloadGameCodeDll()
{
	if (!game_code_dll) return FALSE;
	assert(game_code_dll);
	int result{ FreeLibrary(game_code_dll) };
	assert(result);
	game_code_dll = nullptr;
	return TRUE;
}

