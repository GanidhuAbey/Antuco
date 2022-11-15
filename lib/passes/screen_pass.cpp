#include <passes/screen_pass.hpp>

using namespace pass;

ScreenPass::ScreenPass() {
	// get screen size
	builder::Dimensions dim = FrameBuilder::get()->get_window_dimensions();

	builder::ImageResource color_image{};
	color_image.type = COLOR_IMAGE;
	color_image.dim = dim;
	color_image.id = 0;

	builder::ImageResource depth_image{};
	depth_image.type = DEPTH_IMAGE;
	depth_image.dim = dim;
	depth_image.id = 0;


	RenderPass(depth_image, color_image);
}

void ScreenPass::initialize() {
	RenderPass::initialize();
}

void ScreenPass::frame_begin() {
	RenderPass::frame_begin();
}

void ScreenPass::write_commands(v::CommandBuffer cmd_buff) {

}
