#pragma once

#include <passes/pass.hpp>
#include <passes/frame_resources.hpp>
#include <passes/frame_builder.hpp>
#include <config.hpp>

#include <vulkan_wrapper/command_buffer.hpp>

#include <optional>

// still require an id for each pass, perhaps its easier to precompute this somewhere else...
namespace pass {

class RenderPass : public Pass {
public:
    // resources
    ImageResource depth_resource;
    ImageResource color_resource;

    builder::Image* depth = nullptr;
    builder::Image* color = nullptr;

public:
    RenderPass(builder::ImageResource depth, builder::ImageResource color) {
        depth_texture = depth;
        color_texture = color;
    }

    void initialize() override;

    void write_commands(v::CommandBuffer cmd_buff) override;

    void set_depth(ImageResource depth);
    void set_color(ImageResource color);
};

}
