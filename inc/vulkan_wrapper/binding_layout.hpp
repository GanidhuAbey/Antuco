#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan_wrapper/device.hpp>
#include <vector>

// Create descriptor set layouts
namespace v {

struct DescriptorSetLayoutData {
  uint32_t set_number;
  VkDescriptorSetLayoutCreateInfo create_info;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class BindingLayouts {
private:
    std::vector<VkDescriptorSetLayout> layouts;

    v::Device& m_device;

public: 
    BindingLayouts(v::Device& device) : m_device(device) {}
    ~BindingLayouts() = default;

    void initialize_layouts(std::vector<DescriptorSetLayoutData>& info);
};

}
