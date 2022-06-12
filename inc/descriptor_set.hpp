#pragma once

#include "logger/interface.hpp"
#include "memory_allocator.hpp"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace tuco {	
class ResourceCollection {
private:
	VkDevice p_device;

	bool buffer_add = false;
	bool image_add = false;

	std::vector<VkDescriptorSet> sets;

	VkDescriptorImageInfo image_info{};
	VkDescriptorBufferInfo buffer_info{};

	VkDescriptorType descriptor_type;

	uint32_t binding;

public:
	void init(VkDevice& device, VkDescriptorType type, VkDescriptorSetLayout layout, size_t resource_size, mem::Pool& pool);

	ResourceCollection() {}
	~ResourceCollection() {}

	void update_set();
	void update_set(size_t i);

	void add_image(VkImageLayout image_layout, VkImageView& image_view, VkSampler& image_sampler);
	void add_buffer(VkBuffer buffer, VkDeviceSize buffer_offset, VkDeviceSize buffer_range);
	void set_binding(uint32_t binding);

	void remove_buffer();
	void remove_image();

	VkDescriptorSet get_api_set(size_t i);

	void create_set(uint32_t size, VkDescriptorSetLayout layout, mem::Pool& pool);

private:
	bool check_size(size_t i);
};
}