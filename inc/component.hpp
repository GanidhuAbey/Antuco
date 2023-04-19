#pragma once

#include <config.hpp>
#include <crossguid/guid.hpp>

namespace tuco 
{

class Component 
{
public:
	CLASS_ID;

private:
	TypeId runtime_id;

public:
	// Every component will have a runtime id used to retrieve specific components from entities.
	// For example, if you have multiple components of the same type attached to the same entity,
	// then having a per-component id will help retrieve the correct id.
	// For now we're doing basic counter to track unique id's. but this always has the risk of 
	// overflowing on complex scenes.
	TypeId get_id() { return runtime_id; };
	void set_id(TypeId id) { runtime_id = id; }

	Component() = default;
	~Component() = default;
};

class TestComponent : public Component 
{
	static uint32_t get_id() { return 1; }
};

}