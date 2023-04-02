#pragma once

namespace tuco 
{

class Component 
{
public:
	static uint32_t get_id() { return 0; }
};

class TestComponent : public Component 
{
	static uint32_t get_id() { return 1; }
};

}

