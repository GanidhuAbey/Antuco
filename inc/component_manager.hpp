#pragma once

#include <unordered_map>
#include <config.hpp>

#include <antuco.hpp>

namespace tuco 
{

class ComponentManager 
{
private:
	static std::unordered_map<uint32_t, std::vector<std::unique_ptr<Component>>> components;
	uint32_t m_component_counter = 0;
public:
	template <typename T>
	static void add_component(uint32_t entity_id) 
	{
		T component;
		component.set_id(T::ID);
		component.set_entity_id(entity_id);

		add_to_entity(std::make_unique<T>(component), entity_id);
	}

	template <typename T>
	static T* get_component(uint32_t entity_id) 
	{
		return static_cast<T*>(get_component_from_entity(entity_id, T::ID));
	}

private:
	// This is not the most performance way to add components, as searching for components will now take linear time.
	// A potentially better way might be to, create separate maps per each component type, and then create a map of maps.
	// such that we have two constant time searches.
	static void add_to_entity(std::unique_ptr<Component> component, uint32_t entity_id)
	{
		auto search = components.find(entity_id);

		if (search == components.end())
		{
			components[entity_id].push_back(std::move(component));
			auto search = components.find(entity_id);
			search->second[0]->activate();
			return;
		}

		search->second.push_back(std::move(component));

		// Activate component
		search->second[search->second.size() - 1]->activate();
	}

	static Component* get_component_from_entity(uint32_t entity_id, TypeId component_id)
	{
		auto search = components.find(entity_id);
		if (search == components.end())
		{
			return nullptr;
		}

		for (int i = 0; i < search->second.size(); i++)
		{
			if (search->second[i]->get_id() == component_id)
			{
				return search->second[i].get();
			}
		}

		return nullptr;
	}
};

}
