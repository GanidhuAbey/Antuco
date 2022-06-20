#include "world_objects.hpp"

#include <glm/ext.hpp>

using namespace tuco;

/// <summary>
/// Creates a directional light source for the scene.
/// [RESTRICTION] - 'light_direction' and 'up' must be linearly independent
/// </summary>
/// <param name="light_pos"> position of the light source </param>
/// <param name="light_direction"> direction the light is facing at </param>
/// <param name="light_color"> the colour the light is emanating </param>
DirectionalLight::DirectionalLight(glm::vec3 light_pos, glm::vec3 light_target, glm::vec3 light_color, glm::vec3 up) {
	position = glm::vec4(light_pos, 0.0);
	target = glm::vec4(light_target, 0.0);
	color = glm::vec4(light_color, 0.0);

	//save orientation
	orientation = up;

	//generate mvp matrix (without model)
	perspective = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 96.0f);
	world_to_light = glm::lookAt(glm::vec3(position), glm::vec3(target), orientation);
}

void DirectionalLight::update(glm::vec3 new_position) {
	position = glm::vec4(new_position, 0.0);

	//update mvp matrix
	world_to_light = glm::lookAt(glm::vec3(position), glm::vec3(target), orientation);
}

DirectionalLight::~DirectionalLight() {

}
