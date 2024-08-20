#pragma once

#include <vulkan/vulkan.hpp>

#include <vulkan_wrapper/device.hpp>

namespace v
{

class DescriptorLayout
{
private:
	VkDescriptorSetLayout layout;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	uint32_t index;

	std::shared_ptr<v::Device> p_device;

public:
	void add_binding(VkDescriptorSetLayoutBinding binding) { bindings.push_back(binding); }
	VkDescriptorSetLayoutBinding& get_binding(uint32_t index) { return bindings[index]; }

	void set_index(uint32_t index) { DescriptorLayout::index = index; }
	uint32_t get_index() { return index; }

	void build(std::shared_ptr<v::Device> device);
};

}