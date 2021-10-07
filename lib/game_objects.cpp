#include "world_objects.hpp"
#include "model.hpp"

using namespace tuco;

GameObject::GameObject() {
	object_model = Model();
	transform = glm::mat4(0);
}

GameObject::~GameObject() {}

void GameObject::add_mesh(const std::string& file_name) {
	changed = 1;
	object_model.add_mesh(file_name);
}
