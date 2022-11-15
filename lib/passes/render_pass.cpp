#include <passes/render_pass.hpp>

using namespace pass;

void RenderPass::initialize() {
	depth = FrameBuilder::get()->find_or_create(depth_resource);
	color = FrameBuilder::get()->find_or_create(color_resource);


}

void RenderPass::set_depth(ImageResource depth) {
	depth_resource = depth;
}

void RenderPass::set_color(ImageResource color) {
	color_resource = color;
}