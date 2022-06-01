#include "mesh.hpp"

using namespace tuco;


Mesh::Mesh(std::vector<Vertex> newVertices, std::vector<uint32_t> newIndices, Material material) {
	vertices = newVertices;
	indices = newIndices;

    glm::vec4 texture_opacity;
    if (material.texturePath.has_value()) {
        textures.push_back(material.texturePath.value());
        texture_opacity = glm::vec4(1.0, material.opacity, 0.0, 0.0);
    } else {
        texture_opacity = glm::vec4(0.0, material.opacity, 0.0, 0.0); 
    }

    glm::vec4 ambient_colour = glm::vec4(material.ambient.r,material.ambient.b,material.ambient.g, 0.0);
    glm::vec4 diffuse_colour = glm::vec4(material.diffuse.r,material.diffuse.b,material.diffuse.g, 0.0);
    glm::vec4 specular_colour = glm::vec4(material.specular.r,material.specular.b,material.specular.g, 0.0);

    mat_data.init(texture_opacity, ambient_colour, diffuse_colour, specular_colour);
}

Mesh::Mesh(std::vector<Vertex> new_vertices, std::vector<uint32_t> new_indices, MaterialsObject mat_obj) {
    vertices = new_vertices;
    indices = new_indices;
    mat_data = mat_obj;
}

Mesh::~Mesh() {}

bool Mesh::is_transparent() {
    if (mat_data.texture_opacity.g < 0.999) {
        return true;
    }

    return false;
}
