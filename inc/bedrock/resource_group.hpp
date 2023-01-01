#pragma once

#include "logger/interface.hpp"
#include "memory_allocator.hpp"
#include "vulkan_wrapper/device.hpp"

#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

namespace br {	

enum ResourceType {
	Image = (uint32_t)vk::DescriptorType::eCombinedImageSampler,
	ReadBuffer = (uint32_t)vk::DescriptorType::eUniformBuffer,
	ReadWriteBuffer = (uint32_t)vk::DescriptorType::eStorageBuffer
};

enum ResourceFrequency {
	Frame = 0, // Bound once for the entire frame.
	Pass = 1, // Bound once for each pass.
	Draw = 2, // Bound once for each draw item.
	Invalid = 9
};

enum ShaderType {
    Vertex = (uint32_t)vk::ShaderStageFlagBits::eVertex,
    Fragment = (uint32_t)vk::ShaderStageFlagBits::eFragment,
    Compute = (uint32_t)vk::ShaderStageFlagBits::eCompute
};

struct ShaderBinding {
    uint32_t bind_slot;
    ResourceType resource_type;
    ShaderType shader_type;
    vk::DescriptorImageInfo* image_info;
    vk::DescriptorBufferInfo* buffer_info;
};

class ResourceGroup {
private:
	void create_set(mem::Pool& pool);
    void create_descriptor_set_layout();

	void destroy();

private:
	v::Device* m_device;
    vk::UniqueDescriptorSetLayout m_layout;

	bool m_buffer_add = false;
	bool m_image_add = false;

	vk::DescriptorSet m_set;

    std::vector<ShaderBinding> m_shader_bindings;

	VkDescriptorType m_descriptor_type;

	uint32_t m_pool_index;

	ResourceFrequency m_frequency = ResourceFrequency::Invalid;

	bool m_built = false;

public:
	void build(
        const v::Device& device,
        mem::Pool& pool);


	ResourceGroup() = default;
	~ResourceGroup() = default;

	void add_resource(ShaderBinding resource);

	void update_set();
	void release_bindings();

	//void add_image(VkImageLayout image_layout, mem::Image& image, vk::Sampler& image_sampler);

    //IMPLICIT: assumes that size of vector is equal to set size, throws error otherwise
	//void add_image(
    //        VkImageLayout image_layout, 
    //        std::vector<mem::Image>& images, 
    //        vk::Sampler& image_sampler);

	//void add_buffer(VkBuffer buffer, VkDeviceSize buffer_offset, VkDeviceSize buffer_range);

	void set_frequency(ResourceFrequency frequency) {
		m_frequency = frequency;
	}

	//Better to have used overwrite entire descriptor set.
	//void remove_buffer();
	//void remove_image();

	//VkDescriptorSet get_api_set(size_t i);

	vk::DescriptorSet& get_set() {
		return m_set;
	}
	vk::DescriptorSetLayout get_layout() {
		return m_layout.get();
	}
};
}
