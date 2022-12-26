#pragma once

#include <passes/render_pass.hpp>
#include <tuco_pass.hpp>
#include <pipeline.hpp>
#include <data_structures.hpp>

namespace pass {
class ShadowPass : public RenderPass {
private:
    uint32_t m_shadowmap_size;

    float m_depth_bias_constant = 3.5f;
    float m_depth_bias_slope = 9.5f;

    tuco::LightObject m_light;

    std::vector<std::unique_ptr<tuco::GameObject>> m_game_objects;

public:
    ShadowPass() = default;
    ~ShadowPass() = default;

    void initialize() override;
    void create_commands(vk::CommandBuffer cmd_buff) override;

    // TODO: next step is to abstract away these methods so they don't have to be defined
    //       for every pass.
    void set_light_push_data(tuco::LightObject light);
    void set_game_objects(std::vector<std::unique_ptr<tuco::GameObject>> m_game_objects);

};
}
