#pragma once

#include <passes/pass.hpp>
#include <passes/frame_resources.hpp>
#include <passes/frame_builder.hpp>
#include <config.hpp>

#include <optional>

// still require an id for each pass, perhaps its easier to precompute this somewhere else...
namespace pass {

class RenderPass : Pass {
public:
    // resources
    builder::ImageResource depth_texture;
    builder::ImageResource color_texture;

public:
    RenderPass(builder::ImageResource depth, builder::ImageResource color) {
        depth_texture = depth;
        color_texture = color;
    }

    void initialize() override;


};

}
