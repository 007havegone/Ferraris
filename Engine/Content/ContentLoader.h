#pragma once
#include "CommonHeaders.h"
#if !defined(SHIPPING)
namespace ferraris::content {
bool load_game();
void unload_game();
bool load_engine_shaders(std::unique_ptr<u8[]>& shaders_blob, u64& size);
}
#endif // !defined(SHIPPING)