#pragma once

#include <component.hpp>
#include <transform_component.hpp>
#include <antuco.hpp>

#include <logger/interface.hpp>

namespace tuco 
{

class Entity 
{
private:
	uint32_t id;
public:
#pragma optimize("", off)
	uint32_t get_id() { return id; }

	Entity(uint32_t entity_id) : id(entity_id) 
	{
		//add_component(tuco::TransformComponent(), tuco::TransformComponent::ID);
	}
	~Entity() = default;

#pragma optimize("", on)
};

}