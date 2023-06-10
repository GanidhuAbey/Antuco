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
	TypeId m_runtime_id;
	uint32_t m_entity_id;

public:
	// Every component will have a runtime id used to retrieve specific components from entities.
	// For example, if you have multiple components of the same type attached to the same entity,
	// then having a per-component id will help retrieve the correct id.
	// For now we're doing basic counter to track unique id's. but this always has the risk of 
	// overflowing on complex scenes.
	TypeId get_id() { return m_runtime_id; };
	void set_id(TypeId id) { m_runtime_id = id; }

	void set_entity_id(uint32_t id) { m_entity_id = id; }
	uint32_t get_entity_id() { return m_entity_id; }

	Component() = default;
	virtual ~Component() = default;

	virtual void activate() = 0;
};

class TestComponent : public Component 
{
	static uint32_t get_id() { return 1; }
};

}
