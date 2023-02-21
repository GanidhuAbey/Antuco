#pragma once

#include <vkwr.hpp>

#include "vulkan_wrapper/device.hpp"

#define ENGINE_BIND_SLOT 0
#define PASS_BIND_SLOT 1
#define DRAW_BIND_SLOT 2
#define MATERIAL_BIND_SLOT 3

namespace tuco {

enum class ShaderType 
{
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Compute = 1 << 2,
};

inline vk::CommandBuffer begin_command_buffer(
v::Device& device, 
vk::CommandPool& command_pool) {
    //create command buffer
    auto alloc = vk::CommandBufferAllocateInfo(
            command_pool,
            vk::CommandBufferLevel::ePrimary,
            1
        );

    auto buffer = vk::CommandBuffer();
    auto result = device.get().allocateCommandBuffers(&alloc, &buffer);
   
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("");
    }

    auto begin_info = vk::CommandBufferBeginInfo();

    buffer.begin(begin_info);

    return buffer;
}

void end_command_buffer(
v::Device& device, 
vk::Queue& queue, 
vk::CommandPool& command_pool, 
vk::CommandBuffer& command_buffer);


inline vk::CommandPool create_command_pool(
v::Device& device, 
uint32_t queue_family) {
	auto pool_info = vk::CommandPoolCreateInfo(
			{},
			queue_family
		);

	return device.get().createCommandPool(pool_info);	
}

}
