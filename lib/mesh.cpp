#include "mesh.hpp"
#include "material.hpp"

#include "logger/interface.hpp"

using namespace tuco;

Mesh::Mesh(std::vector<Vertex> newVertices, std::vector<uint32_t> newIndices) {
  vertices = newVertices;
  indices = newIndices;

  /*
  glm::vec4 texture_opacity;
  if (material.texturePath.has_value()) {
    textures.push_back(std::string(material.texturePath.value().c_str()));
    texture_opacity = glm::vec4(1.0, material.opacity, 0.0, 0.0);
  } else {
    texture_opacity = glm::vec4(0.0, material.opacity, 0.0, 0.0);
  }

  glm::vec4 ambient_colour = glm::vec4(material.ambient.r, material.ambient.b,
                                       material.ambient.g, 0.0);

  glm::vec4 diffuse_colour = glm::vec4(material.diffuse.r, material.diffuse.b,
                                       material.diffuse.g, 0.0);

  glm::vec4 specular_colour = glm::vec4(
      material.specular.r, material.specular.b, material.specular.g, 0.0);
  */
}

Mesh::~Mesh() {}

bool Mesh::is_transparent() {
  // TODO: implement transparancy
  return false;
}
