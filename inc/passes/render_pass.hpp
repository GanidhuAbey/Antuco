#pragma once

#include <vkwr.hpp>

#include <memory_allocator.hpp>
#include <world_objects.hpp>
#include <pipeline.hpp>
#include <tuco_pass.hpp>
#include <vulkan_wrapper/device.hpp>

namespace pass {

class RenderPass {
protected:
    mem::StackBuffer m_vertex_buffer;
    mem::StackBuffer m_index_buffer;

    tuco::TucoPass m_pass;
    tuco::TucoPipeline m_pipeline;
    vk::Framebuffer m_framebuffer;

    v::Device& m_device;
public:
    RenderPass(v::Device& device)
        : m_device(device) {
        
    };
    ~RenderPass() {
        m_device.get().destroyFramebuffer(m_framebuffer);
    };

    // create permanant resources for pass
    virtual void initialize() = 0;

    // record commands which will be run by frame builder.
    virtual void create_commands(vk::CommandBuffer& cmd_buff) = 0;

    //void set_vertex_buffer(mem::StackBuffer& vertex_buffer);
    //void set_index_buffer(mem::StackBuffer& index_buffer);
};

}
