#pragma once

#include "logger/interface.hpp"
#include "memory_allocator.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan_wrapper/device.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

namespace tuco {
class ResourceCollection {
private:
  v::Device *device;

  bool buffer_add = false;
  bool image_add = false;

  std::vector<VkDescriptorSet> sets;
  std::vector<std::vector<VkWriteDescriptorSet>> setsUpdateInfo;
  std::vector<VkDescriptorImageInfo> descriptorImageInfo;
  std::vector<VkDescriptorBufferInfo> descriptorBufferInfo;

  bool built = false;

public:
  void init(const v::Device &device, VkDescriptorSetLayout layout,
            uint32_t setCount, mem::Pool &pool);

  ~ResourceCollection() { destroy(); }

  void updateSets();
  void updateSet(size_t i);

  void addImage(uint32_t binding, VkDescriptorType type,
                VkImageLayout image_layout, mem::Image &image,
                vk::Sampler &image_sampler);

  void addImages(uint32_t binding, VkDescriptorType type,
                 VkImageLayout imageLayout, std::vector<mem::Image> &images,
                 vk::Sampler &imageSampler);

  void addBuffer(uint32_t binding, VkDescriptorType type, VkBuffer buffer,
                 VkDeviceSize buffer_offset, VkDeviceSize buffer_range);

  VkDescriptorSet get_api_set(size_t i);

private:
  void create_set(VkDescriptorSetLayout layout, mem::Pool &pool);
  bool check_size(size_t i);

  void destroy();
};
} // namespace tuco
