#include "descriptor_set.hpp"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

using namespace tuco;

void ResourceCollection::init(const v::Device &device,
                              VkDescriptorSetLayout layout) {
  ResourceCollection::device = const_cast<v::Device *>(&device);
  m_layout = layout;
}

uint32_t ResourceCollection::addSets(uint32_t setCount, mem::Pool &pool) {
  uint32_t firstSetIndex = sets.size();
  sets.resize(sets.size() + setCount);
  setsUpdateInfo.resize(setsUpdateInfo.size() + setCount);
  pool.allocateDescriptorSets(*device, setCount, m_layout,
                              &sets[firstSetIndex]);

  return firstSetIndex;
}

bool ResourceCollection::check_size(size_t i) {
  if (i >= sets.size()) {
    ERR("invalid allocation");
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

void ResourceCollection::createSets(VkDescriptorSetLayout layout,
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

  for (const auto &updateInfo : setsUpdateInfo[i]) {
    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.descriptorType = updateInfo.type;
    writeInfo.descriptorCount = 1;
    writeInfo.dstSet = sets[updateInfo.setIndex];
    writeInfo.dstBinding = updateInfo.binding;
    if (updateInfo.imageInfoIndex != -1) {
      writeInfo.pImageInfo = &descriptorImageInfo[updateInfo.imageInfoIndex];
    }
    if (updateInfo.bufferInfoIndex != -1) {
      writeInfo.pBufferInfo = &descriptorBufferInfo[updateInfo.bufferInfoIndex];
    }
    writeInfo.dstArrayElement = 0;

    vkUpdateDescriptorSets(device->get(), 1, &writeInfo, 0, nullptr);
  }
}

void ResourceCollection::addBuffer(BufferDescription info, uint32_t setIndex) {
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = info.buffer;
  bufferInfo.offset = info.bufferOffset;
  bufferInfo.range = info.bufferRange;

  descriptorBufferInfo.push_back(bufferInfo);

  ResourceWriteInfo writeInfo{};
  writeInfo.binding = info.binding;
  writeInfo.type = info.type;
  writeInfo.bufferInfoIndex = descriptorBufferInfo.size() - 1;

  writeInfo.setIndex = setIndex;
  setsUpdateInfo[setIndex].push_back(writeInfo);
}

void ResourceCollection::addBuffer(uint32_t binding, VkDescriptorType type,
                                   VkBuffer buffer, VkDeviceSize bufferOffset,
                                   VkDeviceSize bufferRange) {
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer;
  bufferInfo.offset = bufferOffset;
  bufferInfo.range = bufferRange;

  descriptorBufferInfo.push_back(bufferInfo);

  ResourceWriteInfo writeInfo{};
  writeInfo.binding = binding;
  writeInfo.type = type;
  writeInfo.bufferInfoIndex = descriptorBufferInfo.size() - 1;

  for (uint32_t i = 0; i < sets.size(); i++) {
    writeInfo.setIndex = i;
    setsUpdateInfo[i].push_back(writeInfo);
  }
}

void ResourceCollection::addBufferPerSet(
    uint32_t binding, VkDescriptorType type, std::vector<VkBuffer> buffers,
    std::vector<VkDeviceSize> bufferOffsets,
    std::vector<VkDeviceSize> bufferRanges) {

  if (buffers.size() != sets.size()) {
    ERR("There should be one buffer for every set when adding "
        "multiple buffers");
    throw std::runtime_error("");
  }

  for (int i = 0; i < buffers.size(); i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers[i];
    bufferInfo.offset = bufferOffsets[i];
    bufferInfo.range = bufferRanges[i];

    descriptorBufferInfo.push_back(bufferInfo);

    ResourceWriteInfo writeInfo{};
    writeInfo.binding = binding;
    writeInfo.type = type;
    writeInfo.bufferInfoIndex = descriptorBufferInfo.size() - 1;
    writeInfo.setIndex = i;

    setsUpdateInfo[i].push_back(writeInfo);
  }
}

void ResourceCollection::addImagePerSet(uint32_t binding, VkDescriptorType type,
                                        VkImageLayout imageLayout,
                                        std::vector<br::Image> &images,
                                        vk::Sampler &imageSampler) {

  if (images.size() != sets.size()) {
    ERR("There should be one image for every set when adding "
        "multiple images");
    throw std::runtime_error("");
  }

  for (uint32_t i = 0; i < images.size(); i++) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = imageLayout;
    imageInfo.imageView = images[i].get_api_image_view();
    imageInfo.sampler = imageSampler;

    descriptorImageInfo.push_back(imageInfo);

    ResourceWriteInfo writeInfo{};
    writeInfo.binding = binding;
    writeInfo.type = type;
    writeInfo.imageInfoIndex = descriptorImageInfo.size() - 1;

    writeInfo.setIndex = i;
    setsUpdateInfo[i].push_back(writeInfo);
  }
}

void ResourceCollection::addImage(uint32_t binding, VkDescriptorType type,
                                  VkImageLayout imageLayout, br::Image &image,
                                  vk::Sampler &imageSampler) {

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = imageLayout;
  imageInfo.imageView = image.get_api_image_view();
  imageInfo.sampler = imageSampler;

  descriptorImageInfo.push_back(imageInfo);

  ResourceWriteInfo writeInfo{};
  writeInfo.binding = binding;
  writeInfo.type = type;
  writeInfo.imageInfoIndex = descriptorImageInfo.size() - 1;

  for (uint32_t i = 0; i < sets.size(); i++) {
    writeInfo.setIndex = i;
    setsUpdateInfo[i].push_back(writeInfo);
  }
}

void ResourceCollection::addImage(ImageDescription info, uint32_t set_index)
{
    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = info.image_layout;
    image_info.imageView = info.image_view;
    image_info.sampler = info.sampler;

    descriptorImageInfo.push_back(image_info);

    ResourceWriteInfo write_info{};
    write_info.binding = info.binding;
    write_info.type = info.type;
    write_info.imageInfoIndex = descriptorImageInfo.size() - 1;

    write_info.setIndex = set_index;
    setsUpdateInfo[set_index].push_back(write_info);
}