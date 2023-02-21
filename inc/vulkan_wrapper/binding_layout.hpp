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
  std::vector<std::string> shader_names; // TODO: string comparison is slow.
};

class BindingLayouts {
private:
    std::vector<VkDescriptorSetLayout> m_layouts;

    v::Device& m_device;

public: 
    BindingLayouts(v::Device& device) : m_device(device) {}
    ~BindingLayouts() = default;

    void initialize_layouts(std::vector<DescriptorSetLayoutData>& info);
    VkDescriptorSetLayout& get_layout(uint32_t index);

    std::vector<VkDescriptorSetLayout>& get_layouts() {
        return m_layouts;
    }
};

}
