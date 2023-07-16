#pragma once

#include <passes/render_pass.hpp>
#include <tuco_pass.hpp>
#include <pipeline.hpp>
#include <data_structures.hpp>

#include <bedrock/draw_call.hpp>

namespace pass {
class ShadowPass : public RenderPass {
private:
    uint32_t m_shadowmap_size;

    float m_depth_bias_constant = 3.5f;
    float m_depth_bias_slope = 9.5f;

    tuco::LightObject m_light;

    std::vector<br::DrawCall*> m_draw_calls;

    v::Device& m_device;

public:
    ShadowPass(v::Device& device) : 
        RenderPass(device),
        m_device(device) {
        
    };

    ~ShadowPass() = default;

    void initialize() override;
    void create_commands(vk::CommandBuffer& cmd_buff) override;

    void add_draw_call(br::DrawCall* draw_call) {
        m_draw_calls.push_back(draw_call);
    }

private:
    void bind_pass_resources();
};
}
