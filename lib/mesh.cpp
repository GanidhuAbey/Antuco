#include "mesh.hpp"

using namespace tuco;


Mesh::Mesh(std::vector<Vertex> newVertices, std::vector<uint32_t> newIndices, Material material) {
	vertices = newVertices;
	indices = newIndices;

    if (material.texturePath.has_value()) {
        textures.push_back(material.texturePath.value());
        mat_data.texture_opacity = glm::vec4(1.0, material.opacity, 0.0, 0.0);
    } else {
        mat_data.texture_opacity = glm::vec4(0.0, material.opacity, 0.0, 0.0); 
    }

    glm::vec4 ambient_colour = glm::vec4(material.ambient.r,material.ambient.b,material.ambient.g, 0.0);
    glm::vec4 diffuse_colour = glm::vec4(material.diffuse.r,material.diffuse.b,material.diffuse.g, 0.0);
    glm::vec4 specular_colour = glm::vec4(material.specular.r,material.specular.b,material.specular.g, 0.0);

    mat_data.ambient = ambient_colour;
    mat_data.diffuse = diffuse_colour;
    mat_data.specular = specular_colour;
}

Mesh::~Mesh() {}

bool Mesh::is_transparent() {
    if (mat_data.texture_opacity.g < 0.999) {
        return true;
    }

    return false;
}