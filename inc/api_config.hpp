#pragma once

#include "vulkan/vulkan.hpp"

#include "vulkan_wrapper/device.hpp"

namespace tuco {

// TODO: this isn't a great method considering if we move around our files
//       the whole thing will break...
const std::string SHADER_PATH = get_project_root(__FILE__) + "/shaders/";
const std::string CACHE_PATH = get_project_root(__FILE__) + "/cache/";

#define SHADER(file) SHADER_PATH + file

const uint32_t MAX_FRAMES_IN_FLIGHT = 3;


inline VkCommandBuffer begin_cmd(v::Device* p_device, vk::CommandPool& command_pool)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;                  // Optional
    begin_info.pInheritanceInfo = nullptr; // Optional

    VkCommandBufferAllocateInfo alloc{};
    alloc.commandBufferCount = 1;
    alloc.commandPool = command_pool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(p_device->get(), &alloc, &cmd);
    vkBeginCommandBuffer(cmd, &begin_info);

    return cmd;
}

inline void end_cmd(v::Device* p_device, vk::Queue& queue, vk::CommandPool& cmd_pool, VkCommandBuffer cmd)
{
    vkEndCommandBuffer(cmd);

    VkSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.pNext = nullptr;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &cmd;
    info.signalSemaphoreCount = 0;
    info.pSignalSemaphores = nullptr;
    info.waitSemaphoreCount = 0;
    info.pWaitSemaphores = nullptr;
    info.pWaitDstStageMask = nullptr;
    vkQueueSubmit(queue, 1, &info, nullptr);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(p_device->get(), cmd_pool, 1, &cmd);
}

inline vk::CommandBuffer cpp_begin_command_buffer(
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

void cpp_end_command_buffer(
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
