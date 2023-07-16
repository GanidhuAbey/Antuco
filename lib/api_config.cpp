#include "api_config.hpp"

void tuco::end_command_buffer(
v::Device& device, 
vk::Queue& queue, 
vk::CommandPool& command_pool, 
vk::CommandBuffer& command_buffer) {
    //end command buffer
    command_buffer.end();

    auto info = vk::SubmitInfo(
            0, nullptr,
            nullptr,
            1,
            &command_buffer,
            0, nullptr
        );

    queue.submit(info);
    queue.waitIdle();

    device.get().free(command_pool, command_buffer);
}

 
