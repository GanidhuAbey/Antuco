#pragma once

#include <component.hpp>
#include <glm/glm.hpp>

namespace tuco 
{

class TransformComponent : public Component 
{
private:
	const static uint32_t component_id = 1917922934;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

public:
	uint32_t get_id() 
	{
		return component_id;
	}

	TransformComponent() = default;
	~TransformComponent() = default;

	uint32_t translate(glm::vec3 translate_vector) 
	{
		position += translate_vector;
	}

	uint32_t euclidean_rotate(glm::vec3 rotate_vector) 
	{
		rotation += rotate_vector;
	}

	uint32_t scale(glm::vec3 scale_vector) 
	{
		scale += scale_vector;
	}
};

}