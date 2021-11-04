#include "world_objects.hpp"

using namespace tuco;

Light::Light(glm::vec3 light_pos, glm::vec3 light_color) {
	position = light_pos;
	color = light_color;
}

void Light::update(glm::vec3 translation) {
	position = translation;
}

Light::~Light() {

}