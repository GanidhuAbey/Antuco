#include "world_objects.hpp"
#include "model.hpp"

using namespace tuco;

GameObject::GameObject() {
	object_model = Model();
	transform = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
}

GameObject::~GameObject() {}

void GameObject::add_mesh(const std::string& file_name) {
	changed = 1;
	object_model.add_mesh(file_name);
}

void GameObject::translate(glm::vec3 t) {
	glm::mat4 translation = {
		1, 0, 0, t.x,
		0, 1, 0, t.y,
		0, 0, 1, t.z,
		0, 0, 0, 1
	};

	transform = glm::transpose(translation) * transform;
}

void GameObject::scale(glm::vec3 scale_vector) {
	glm::mat4 scale_mat = {
		scale_vector.x, 0, 0, 0,
		0, scale_vector.y, 0, 0,
		0, 0, scale_vector.z, 0,
		0, 0, 0, 1
	};

	transform = transform * glm::transpose(scale_mat);
}
