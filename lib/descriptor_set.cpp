#include "descriptor_set.hpp"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

using namespace tuco;

void ResourceCollection::init(const v::Device &device,
                              VkDescriptorSetLayout layout, uint32_t setCount,
                              mem::Pool &pool) {
  ResourceCollection::device = const_cast<v::Device *>(&device);

  sets.resize(setCount);
  setsUpdateInfo.resize(setCount);

  create_set(layout, pool);
}

bool ResourceCollection::check_size(size_t i) {
  if (i >= sets.size()) {
    LOG("[ERROR] - invalid allocation");
    throw std::runtime_error("exection forcefully stopped");
    return false;
  }

  return true;
}

VkDescriptorSet ResourceCollection::get_api_set(size_t i) {
  if (!check_size(i)) {
    return nullptr;
  }

  return sets[i];
}

void ResourceCollection::destroy() {}

void ResourceCollection::create_set(VkDescriptorSetLayout layout,
                                    mem::Pool &pool) {
  pool.allocateDescriptorSets(*device, sets.size(), layout, sets.data());
}

void ResourceCollection::updateSets() {
  for (size_t i = 0; i < sets.size(); i++) {
    updateSet(i);
  }
}

void ResourceCollection::updateSet(size_t i) {
  if (!check_size(i)) {
    return;
  }

  for (auto writeInfo : setsUpdateInfo[i]) {
    vkUpdateDescriptorSets(device->get(), 1, &writeInfo, 0, nullptr);
  }
}

void ResourceCollection::addBuffer(uint32_t binding, VkDescriptorType type,
                                   VkBuffer buffer, VkDeviceSize bufferOffset,
                                   VkDeviceSize bufferRange) {
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer;
  bufferInfo.offset = bufferOffset;
  bufferInfo.range = bufferRange;

  descriptorBufferInfo.push_back(bufferInfo);

  VkWriteDescriptorSet bufferWrite{};
  bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  bufferWrite.descriptorCount = 1;
  bufferWrite.dstBinding = binding;
  bufferWrite.descriptorType = type;
  bufferWrite.pBufferInfo =
      &descriptorBufferInfo[descriptorBufferInfo.size() - 1];
  bufferWrite.dstArrayElement = 0;

  for (uint32_t i = 0; i < sets.size(); i++) {
    bufferWrite.dstSet = sets[i];
    setsUpdateInfo[i].push_back(bufferWrite);
  }
}

void ResourceCollection::addImages(uint32_t binding, VkDescriptorType type,
                                   VkImageLayout imageLayout,
                                   std::vector<mem::Image> &images,
                                   vk::Sampler &imageSampler) {

  if (images.size() != sets.size()) {
    LOG("[ERROR] - there should be one image for every set when adding "
        "multiple images");
    throw std::runtime_error("");
  }

  for (uint32_t i = 0; i < images.size(); i++) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = imageLayout;
    imageInfo.imageView = images[i].get_api_image_view();
    imageInfo.sampler = imageSampler;

    descriptorImageInfo.push_back(imageInfo);

    VkWriteDescriptorSet writeImage{};
    writeImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeImage.descriptorType = type;
    writeImage.descriptorCount = 1;
    writeImage.dstBinding = binding;
    writeImage.pImageInfo =
        &descriptorImageInfo[descriptorImageInfo.size() - 1];
    writeImage.dstArrayElement = 0;

    writeImage.dstSet = sets[i];
    setsUpdateInfo[i].push_back(writeImage);
  }
}

void ResourceCollection::addImage(uint32_t binding, VkDescriptorType type,
                                  VkImageLayout imageLayout, mem::Image &image,
                                  vk::Sampler &imageSampler) {

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = imageLayout;
  imageInfo.imageView = image.get_api_image_view();
  imageInfo.sampler = imageSampler;

  descriptorImageInfo.push_back(imageInfo);

  VkWriteDescriptorSet writeImage{};
  writeImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeImage.descriptorType = type;
  writeImage.descriptorCount = 1;
  writeImage.dstBinding = binding;
  writeImage.pImageInfo = &descriptorImageInfo[descriptorImageInfo.size() - 1];
  writeImage.dstArrayElement = 0;

  for (uint32_t i = 0; i < sets.size(); i++) {
    writeImage.dstSet = sets[i];
    setsUpdateInfo[i].push_back(writeImage);
  }
}
