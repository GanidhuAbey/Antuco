#include <passes/render_pass.hpp>

using namespace pass;

void RenderPass::initialize() {
    FrameBuilder::get().create_image_resource(depth_texture);
    FrameBuilder::get().create_image_resource(color_texture);
}