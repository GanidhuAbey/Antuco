#pragma once

#include "logger/interface.hpp"
#include "memory_allocator.hpp"
#include "vulkan_wrapper/device.hpp"

#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

namespace tuco {	

enum ResourceFrequency {
	Frame = 0, // Bound once for the entire frame.
	Pass = 1, // Bound once for each pass.
	Draw = 2, // Bound once for each draw item.
};

class ResourceCollection {
private:
	v::Device* device;

	bool buffer_add = false;
	bool image_add = false;

	VkDescriptorSet set;

    std::vector<VkDescriptorImageInfo> image_info{};
	VkDescriptorBufferInfo buffer_info{};

	VkDescriptorType descriptor_type;

	uint32_t pool_index;

	uint32_t binding;

	bool built = false;

public:
	void init(
            const v::Device& device, 
            VkDescriptorType type, 
            VkDescriptorSetLayout layout, 
            uint32_t resource_size, 
            mem::Pool& pool);

	~ResourceCollection() {
		destroy();
	}

	void update_set();
	void update_set(size_t i);

	void add_image(VkImageLayout image_layout, mem::Image& image, vk::Sampler& image_sampler);

    //IMPLICIT: assumes that size of vector is equal to set size, throws error otherwise
	void add_image(
            VkImageLayout image_layout, 
            std::vector<mem::Image>& images, 
            vk::Sampler& image_sampler);

	void add_buffer(VkBuffer buffer, VkDeviceSize buffer_offset, VkDeviceSize buffer_range);
	void set_binding(uint32_t binding);

	void remove_buffer();
	void remove_image();

	VkDescriptorSet get_api_set(size_t i);

private:
	void create_set(VkDescriptorSetLayout layout, mem::Pool& pool);
	bool check_size(size_t i);

	void destroy();
};
}
