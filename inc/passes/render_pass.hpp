#pragma once

#include <vulkan/vulkan.hpp>

#include <memory_allocator.hpp>
#include <world_objects.hpp>
#include <pipeline.hpp>
#include <tuco_pass.hpp>

namespace pass {

class RenderPass {
protected:
    mem::StackBuffer m_vertex_buffer;
    mem::StackBuffer m_index_buffer;

    tuco::TucoPass m_pass;
    tuco::TucoPipeline m_pipeline;
    vk::Framebuffer m_framebuffer;
    
public:
    RenderPass();
    ~RenderPass();
public:
    // create permanant resources for pass
    virtual void initialize();

    // record commands which will be run by frame builder.
    virtual void create_commands(vk::CommandBuffer cmd_buff);

    void set_vertex_buffer(mem::StackBuffer& vertex_buffer);
    void set_index_buffer(mem::StackBuffer& index_buffer);
};

}
