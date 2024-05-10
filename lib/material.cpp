#include "material.hpp"

using namespace tuco;

void Material::setBaseColorTexture(std::string filePath) {
    baseColorTexturePath = filePath;
    hasBaseTexture = true;
}

MaterialBufferObject Material::convert() {
  MaterialBufferObject obj{};
  obj.pbrParameters.x = baseReflectivity;
  obj.pbrParameters.y = roughness;
  obj.pbrParameters.z = metallic;

  obj.hasTexture.x = static_cast<float>(hasBaseTexture);
  obj.hasTexture.y = static_cast<float>(hasMetallicTexture);
  obj.hasTexture.z = static_cast<float>(hasRoughnessTexture);

  obj.albedo.x = albedo.x;
  obj.albedo.y = albedo.y;
  obj.albedo.z = albedo.z;

  return obj;
}

std::vector<float> MaterialBufferObject::linearize() {
  std::vector<float> floats;

  floats.push_back(albedo.x);
  floats.push_back(albedo.y);
  floats.push_back(albedo.z);
  floats.push_back(albedo.w); // padding

  floats.push_back(pbrParameters.x); // baseReflectivity
  floats.push_back(pbrParameters.y); // roughness
  floats.push_back(pbrParameters.z); // metallic
  floats.push_back(pbrParameters.w); // padding

  return floats;
}
