#pragma once

#include <component.hpp>

namespace tuco
{

// forward declare ComponentProcessor
class MeshProcessor;

class MeshComponent : public Component
{
private:
	const static uint32_t component_id = 2626589874;

	MeshProcessor processor;

public:
	uint32_t get_id() { return component_id; }

	MeshComponent();
	~MeshComponent();

	void add_mesh(const std::string& file_name);

};

}