#include <passes/shadow_pass.hpp>

using namespace pass;

ShadowPass::ShadowPass() {
	ImageResource depth{};
	depth.type = DEPTH_IMAGE;
	depth.dim.width = s_shadow_pass_size;
	depth.dim.height = s_shadow_pass_size;
	depth.dim.depth = 1;
	depth.id = 0;


	ImageResource color{};
	color.type = NO_IMAGE;
	
	RenderPass(depth, color);
}

void ShadowPass::initialize() {
	 
}