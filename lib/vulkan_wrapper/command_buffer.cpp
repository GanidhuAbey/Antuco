#include <vulkan_wrapper/command_buffer.hpp>

using namespace v;

CommandBuffer::CommandBuffer(
    Device& device, 
    vk::CommandPool* command_pool, 
    uint32_t submit_count) 
{
    VkCommandBufferAllocateInfo bufferAllocate{};
    bufferAllocate.sType = 
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocate.commandPool = *command_pool;
    bufferAllocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    bufferAllocate.commandBufferCount = submit_count;

    auto allocate_info = vk::CommandBufferAllocateInfo(
        *command_pool,
        vk::CommandBufferLevel::ePrimary,
        submit_count
    );

    command_buffers = device.get()
        .allocateCommandBuffers(allocate_info);
}
