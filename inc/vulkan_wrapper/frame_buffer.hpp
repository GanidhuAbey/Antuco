#pragma once

#include <memory_allocator.hpp>

namespace v {

class FrameBuffer {
    
vk::Framebuffer create_frame_buffer(
    vk::RenderPass pass, 
    uint32_t attachment_count, 
    vk::ImageView* p_attachments, 
    uint32_t width, uint32_t height);
};

}
