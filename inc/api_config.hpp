#pragma once

#include "vulkan/vulkan.hpp"

#include "vulkan_wrapper/device.hpp"

namespace tuco {

// TODO: this isn't a great method considering if we move around our files
//       the whole thing will break...
const std::string SHADER_PATH = get_project_root(__FILE__) + "/shaders/";

#define SHADER(file) SHADER_PATH + file

const uint32_t MAX_FRAMES_IN_FLIGHT = 3;


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
