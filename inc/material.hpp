#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include <vulkan/vulkan.h>

#include <bedrock/image.hpp>

namespace tuco {

struct MaterialGpuInfo {
  VkDeviceSize bufferOffset;
  uint32_t setIndex;
};

struct MaterialBufferObject {
  // baseReflectivity, roughness, metallic
  glm::vec4 pbrParameters = glm::vec4(0.0);
  glm::vec4 hasTexture = glm::vec4(0.0); // hasBaseTexture, hasMetallic, hasRoughness
  glm::vec4 albedo = glm::vec4(0.0);

  glm::vec4 padding[2] = {glm::vec4(0), glm::vec4(0)};

  std::vector<float> linearize();
};

class Material {
private:
	br::Image baseColorImage;
	br::Image roughnessMetallicTexture;
public:
	// uint32_t image_index;
	// std::optional<std::string> texturePath;
	std::string baseColorTexturePath = "";

	float baseReflectivity = 0.04;
	float roughness = 1.0f;
	float metallic = 1.0f;

	bool hasBaseTexture = false;
	bool hasRoughnessTexture = false;
	bool hasMetallicTexture = false;

	glm::vec3 albedo = glm::vec3(1.0);

	MaterialGpuInfo gpuInfo;

	public:
	Material() {}
	~Material() {}
	MaterialBufferObject convert();

	void setBaseColorTexture(std::string filePath);
	br::Image& getBaseColorImage() { return baseColorImage; }
	
	// [TODO] - If roughness and metallic textures are provided separately, then they will need to be
	// combined into a single texture before passing to shader.
	//void setRoughnessTexture(std::string filePath);
	//br::Image& getRoughnessImage() { return roughnessImage; }

	//void setMetallicTexture(std::string filePath);
	//br::Image& getMetallicImage() { return metallicimage; }

	void setRoughnessMetallicTexture(std::string filePath);
	br::Image& getRoughnessMetallicImage() { return roughnessMetallicTexture; }
};

} // namespace tuco
