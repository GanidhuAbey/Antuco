#include "material.hpp"

#include <bedrock/image.hpp>

#include <logger/interface.hpp>
#include <antuco.hpp>
#include <api_graphics.hpp>
#include <vulkan_wrapper/limits.hpp>

#define MATERIAL_SET_INDEX 2

using namespace tuco;

void Material::init()
{
	ResourceCollection* forward_collection = Antuco::get_engine().get_backend()->get_forward_pipeline().get_resource_collection(MATERIAL_SET_INDEX);
	gpuInfo.setIndex = forward_collection->addSets(1, *Antuco::get_engine().get_backend()->get_set_pool());
	gpuInfo.bufferOffset = Antuco::get_engine().get_backend()->get_model_buffer().allocate(sizeof(MaterialBufferObject), v::Limits::get().uniformBufferOffsetAlignment);
}

void Material::setBaseColorTexture(std::string filePath) {
    baseColorTexturePath = filePath;
    hasBaseTexture = true;

    baseColorImage.init("baseColorTexture");
    baseColorImage.load_image(baseColorTexturePath, br::ImageFormat::RGBA_COLOR, br::ImageType::Image_2D);
    baseColorImage.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

//void Material::setRoughnessTexture(std::string filePath) {
//    hasRoughnessTexture = true;
//
//    roughnessImage.init("roughnessTexture");
//    roughnessImage.load_image(filePath, br::ImageFormat::RGBA_COLOR);
//    roughnessImage.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
//}
//

void Material::setRoughnessMetallicTexture(std::string filePath)
{
	hasRoughnessTexture = true;

	roughnessMetallicTexture.init("roughnessMetallicTexture");
	roughnessMetallicTexture.load_image(filePath, br::ImageFormat::RGBA_COLOR, br::ImageType::Image_2D);
	roughnessMetallicTexture.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

void Material::setMetallicTexture(std::string& filePath)
{
	hasSeparateMetallic = true;

	metallicTexture.init("metallicTexture");
	metallicTexture.load_image(filePath, br::ImageFormat::RGBA_COLOR, br::ImageType::Image_2D);
	metallicTexture.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

void Material::UpdateGPU()
{
	WARN("Update GPU data");
}

//void Material::setMetallicTexture(std::string filePath) {}

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
