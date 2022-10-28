#pragma once
#include "ComponentsCommon.h"
namespace ferraris::script {

struct init_info
{
	// the function pointer to creator function or script function
	detail::script_creator script_creator;
};

component create(const init_info& info, game_entity::entity entity);
void remove(component c);
void update(float dt);
}