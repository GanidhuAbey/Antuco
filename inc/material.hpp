#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace tuco {

struct MaterialBufferObject {
  // baseReflectivity, roughness, metallic
  glm::vec4 pbrParameters = glm::vec4(0.0);
  glm::vec4 albedo = glm::vec4(0.0);

  std::vector<float> linearize();
};

class Material {
public:
  // uint32_t image_index;
  // std::optional<std::string> texturePath;

  float baseReflectivity = 0.04;
  float roughness = 1.0f;
  float metallic = 1.0f;

  glm::vec3 albedo = glm::vec3(1.0);

  // signifies where the material is written to in memory.
  uint32_t mem_access = 0;

public:
  Material() {}
  ~Material() {}
  MaterialBufferObject convert();
};

} // namespace tuco
