#include <passes/shadow_pass.hpp>

using namespace pass;

void ShadowPass::initialize() {
    // create PSO

    // create depth resources
    return;
}

void ShadowPass::create_commands(vk::CommandBuffer& cmd_buff) {
    VkRect2D renderArea{};
    renderArea.offset = VkOffset2D{ 0, 0 };
    renderArea.extent = VkExtent2D{ m_shadowmap_size, m_shadowmap_size };

    auto depth_stencil = vk::ClearDepthStencilValue(1.0f, 0);
    auto shadowpass_clear = vk::ClearValue(depth_stencil);

    auto shadowpass_info = vk::RenderPassBeginInfo(
        m_pass.get_api_pass(),
        m_framebuffer,
        renderArea,
        shadowpass_clear
        );

    // TODO: need better description for what this is for.
    int current_shadow = 0;

    //NOTE: when imeplementing multiple shadowmaps, we can loop through MAX_SHADOW_CASTERS twice.
    //for (size_t y = 0; y < glm::sqrt(MAX_SHADOW_CASTERS); y++) {
    //    for (size_t x = 0; x < glm::sqrt(MAX_SHADOW_CASTERS); x++) {
            //there is at least one light that is casting a shadow here

    cmd_buff.beginRenderPass(
        shadowpass_info,
        vk::SubpassContents::eInline
    );

    VkViewport shadowpass_port = {};
    shadowpass_port.x = 0;
    shadowpass_port.y = 0;
    shadowpass_port.width = (float)m_shadowmap_size;
    shadowpass_port.height = (float)m_shadowmap_size;
    shadowpass_port.minDepth = 0.0;
    shadowpass_port.maxDepth = 1.0;
    vkCmdSetViewport(cmd_buff, 0, 1, &shadowpass_port);

    VkRect2D shadowpass_scissor{};
    shadowpass_scissor.offset = { 0, 0 };
    shadowpass_scissor.extent.width = m_shadowmap_size;
    shadowpass_scissor.extent.height = m_shadowmap_size;
    vkCmdSetScissor(cmd_buff, 0, 1, &shadowpass_scissor);

    vkCmdSetDepthBias(
            cmd_buff, 
            m_depth_bias_constant, 
            0.0f, 
            m_depth_bias_slope);

    vkCmdBindPipeline(
            cmd_buff, 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            m_pipeline.get_api_pipeline());

    //time for the draw calls
    const VkDeviceSize offsets[] = { 
        0, 
        offsetof(tuco::Vertex, normal), 
        offsetof(tuco::Vertex, tex_coord) 
    };

    cmd_buff.bindVertexBuffers(
        0, 
        1, 
        &m_vertex_buffer.buffer, 
        offsets
    );

    // TODO: create list of draw items, and offset index buffer by
    //       to location of the objects we're drawing. Gives ability
    //       to use single index buffer for all draw commands.
    cmd_buff.bindIndexBuffer(
        m_index_buffer.buffer,
        0,
        vk::IndexType::eUint32
    );

    uint32_t total_indexes = 0;
    uint32_t total_vertices = 0;

    uint32_t* number;

    cmd_buff.pushConstants(
        m_pipeline.get_api_layout(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(m_light),
        &m_light
    );

    bind_pass_resources();

    for (auto& draw_call : m_draw_calls) {
        cmd_buff.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_pipeline.get_api_layout(),
            0,
            draw_call->get_draw_resources().get_set(),
            {}
        );

        cmd_buff.drawIndexed(
            draw_call->get_index_count(),
            1,
            draw_call->get_index_offset(),
            draw_call->get_vertex_offset(),
            0
        );
    }
    
    cmd_buff.endRenderPass();
}

void ShadowPass::bind_pass_resources() {
    //...
}
