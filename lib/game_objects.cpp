#include "world_objects.hpp"
#include "model.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <antuco.hpp>
#include <api_graphics.hpp>

using namespace tuco;

GameObject::GameObject() : object_model() {
	transform = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
}

GameObject::~GameObject() {}

void GameObject::add_mesh(const std::string& file_name, std::optional<std::string> name) {
	changed = 1;
	object_model.add_mesh(file_name, name);
}

void GameObject::translate(glm::vec3 t) {
	glm::mat4 translation = {
		1, 0, 0, t.x,
		0, 1, 0, t.y,
		0, 0, 1, t.z,
		0, 0, 0, 1
	};

	transform = glm::translate(transform, t);
}

void GameObject::set_position(glm::vec3 t) {
	glm::mat4 translation = {
		1, 0, 0, t.x,
		0, 1, 0, t.y,
		0, 0, 1, t.z,
		0, 0, 0, 1
	};
}

Material* GameObject::get_material()
{
	GraphicsImpl* backend = Antuco::get_engine().get_backend();
	return backend->materials[material_index].get();
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
