#pragma once

#include <vulkan_wrapper/device.hpp>
#include <vulkan/vulkan.hpp>
#include <vector>

namespace v {

class CommandBuffer {
private:
    std::vector<vk::CommandBuffer> command_buffers;
public:
    CommandBuffer(
        Device& device, 
        vk::CommandPool* command_pool,
        uint32_t submit_count);

    ~CommandBuffer() = default;
};

}
