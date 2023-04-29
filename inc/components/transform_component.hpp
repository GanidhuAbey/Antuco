#pragma once

#include <component.hpp>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

namespace tuco 
{

class TransformComponent : public Component 
{
private:
	const static uint32_t component_id = 1917922934;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	glm::mat4 m_modelMatrix;

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
		m_modelMatrix = glm::translate(m_modelMatrix, translate_vector);
	}

	// Rotation not supported yet because i'm lazy.
	uint32_t euclidean_rotate(glm::vec3 rotate_vector) 
	{
		rotation += rotate_vector;
	}

	uint32_t scale(glm::vec3 scale_vector) 
	{
		scale += scale_vector;
		m_modelMatrix = glm::scale(m_modelMatrix, scale_vector);
	}

	glm::mat4& get_model()
	{
		return m_modelMatrix;
	}
};

}