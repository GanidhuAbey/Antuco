#pragma once

#include <component.hpp>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

namespace tuco 
{

class TransformComponent : public Component 
{
public:
	CLASS_ID;
private:
	const static uint32_t component_id = 1917922934;

	glm::vec3 m_position_vector;
	glm::vec3 m_rotation_vector;
	glm::vec3 m_scale_vector;

	glm::mat4 m_modelMatrix;

public:
	uint32_t get_id() 
	{
		return component_id;
	}

	TransformComponent() = default;
	~TransformComponent() = default;

	void translate(glm::vec3 translate_vector) 
	{
		m_position_vector += translate_vector;
		m_modelMatrix = glm::translate(m_modelMatrix, translate_vector);
	}

	// Rotation not supported yet because i'm lazy.
	void euclidean_rotate(glm::vec3 rotate_vector) 
	{
		m_rotation_vector += rotate_vector;
	}

	void scale(glm::vec3 scale_vector) 
	{
		m_scale_vector += scale_vector;
		m_modelMatrix = glm::scale(m_modelMatrix, scale_vector);
	}

	glm::mat4& get_model()
	{
		return m_modelMatrix;
	}

	//tuco::Component overrides...
	void activate() override;
};

}
