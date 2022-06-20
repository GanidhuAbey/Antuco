#include "world_objects.hpp"

using namespace tuco;

PointLight::PointLight(glm::vec3 position, glm::vec3 colour) {
    PointLight::pos = position;
    PointLight::colour = colour;
}
PointLight::~PointLight() {}
