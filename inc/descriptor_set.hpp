#pragma once

#include "logger/interface.hpp"
#include "memory_allocator.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan_wrapper/device.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

namespace tuco {

struct ResourceWriteInfo {
  uint32_t binding;
  uint32_t setIndex;
  VkDescriptorType type;
  // IMPORTANT: only one of these indices should be set.
  int32_t imageInfoIndex = -1;
  int32_t bufferInfoIndex = -1;
};

struct BufferDescription {
  uint32_t binding;
  VkDescriptorType type;
  VkBuffer buffer;
  VkDeviceSize bufferOffset;
  VkDeviceSize bufferRange;
};

class ResourceCollection {
private:
  v::Device *device;

  bool buffer_add = false;
  bool image_add = false;

  std::vector<VkDescriptorSet> sets;
  std::vector<std::vector<ResourceWriteInfo>> setsUpdateInfo;
  std::vector<VkDescriptorImageInfo> descriptorImageInfo;
  std::vector<VkDescriptorBufferInfo> descriptorBufferInfo;

  // A resource collection should only hold sets of the same type (i.e same
  // layout)
  VkDescriptorSetLayout m_layout;

  bool built = false;

public:
  void init(const v::Device &device, VkDescriptorSetLayout layout);

  uint32_t getSetCount() { return sets.size(); }

  ~ResourceCollection() { destroy(); }

  void updateSets();
  void updateSet(size_t i);
  uint32_t addSets(uint32_t setCount, mem::Pool &pool);

  void addImage(uint32_t binding, VkDescriptorType type,
                VkImageLayout image_layout, mem::Image &image,
                vk::Sampler &image_sampler);

  void addImagePerSet(uint32_t binding, VkDescriptorType type,
                      VkImageLayout imageLayout,
                      std::vector<mem::Image> &images,
                      vk::Sampler &imageSampler);

  void addBuffer(uint32_t binding, VkDescriptorType type, VkBuffer buffer,
                 VkDeviceSize buffer_offset, VkDeviceSize buffer_range);

  void addBuffer(BufferDescription info, uint32_t setIndex);

  void addBufferPerSet(uint32_t binding, VkDescriptorType type,
                       std::vector<VkBuffer> buffers,
                       std::vector<VkDeviceSize> bufferOffsets,
                       std::vector<VkDeviceSize> bufferRanges);

  VkDescriptorSet get_api_set(size_t i);

private:
  void createSets(VkDescriptorSetLayout layout, mem::Pool &pool);
  bool check_size(size_t i);

  void destroy();
};
} // namespace tuco
