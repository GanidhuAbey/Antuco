#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

#define POOL_MAX_SET_COUNT 1000

namespace v {

class Limits
{
public:
	uint32_t uniformBufferOffsetAlignment = 0;

	VkDescriptorType descriptor_types[2] = {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
	};

public:
	static Limits& get() {
		return m_instance;
	}
	Limits(const Limits&) = delete;
private:
	Limits();
	~Limits();

	static Limits m_instance;
	
};

}