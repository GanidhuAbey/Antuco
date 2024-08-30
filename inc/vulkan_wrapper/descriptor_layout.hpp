#pragma once

#include <vulkan/vulkan.hpp>

#include <vulkan_wrapper/device.hpp>


// [TODO 8/2024] - Update descriptor layout once we've grasped an idea of how creating & updating descriptor sets will work.
namespace v
{

struct Binding
{
	std::string name;
	VkDescriptorSetLayoutBinding info;
};

class DescriptorLayout
{
private:
	VkDescriptorSetLayout layout;
	std::vector<Binding> bindings;

	uint32_t index;

	std::shared_ptr<v::Device> p_device;

	bool built = false;

public:
	void add_binding(Binding binding) { bindings.push_back(binding); }
	Binding& get_binding(uint32_t index) { return bindings[index]; }

	void set_index(uint32_t index) { DescriptorLayout::index = index; }
	uint32_t get_index() { return index; }

	VkDescriptorSetLayout& get_api_layout() { return layout; }

	void build(std::shared_ptr<v::Device> device);

	DescriptorLayout() = default;
	~DescriptorLayout();
};

}