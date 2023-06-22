#pragma once
#include "Test.h"


// Here we need to add the abstract layer to hidden 
// the implementation of different Rendering API, with help of the render_surface structure

class engine_test : public test
{
public:
	bool initialize() override;

	void run() override;

	void shutdown() override;
};
