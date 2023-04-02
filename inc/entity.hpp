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
	Entity(uint32_t entity_id) : id(entity_id) 
	{
		add_component(tuco::TransformComponent());
	}
	~Entity() = default;

	void add_component(Component&& component) 
	{
		Antuco::get_engine().add_component(std::move(component), id);
	}

	Component* get_component(uint32_t component_id) 
	{
		auto component = Antuco::get_engine().get_component(id, component_id);
		assert(component);
		return component;
	}
};

}