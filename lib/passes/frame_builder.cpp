#include <passes/frame_builder.hpp>

using namespace pass;


// If this doesn't work we can move initialization into a different static
// function and call it during engine initialization.
FrameBuilder *FrameBuilder::get() {
    if (!p_frame_builder) {
        p_frame_builder = std::make_unique<FrameBuilder>();
    }

    return p_frame_builder.get();
}

// per pass data -> descriptor sets
// draw items -> vkDraw

// compute passes will contain no per object data since objects are not submitted
// but it is a much simpler case to handle empty data

void FrameBuilder::build_frame() {
}


// If current pass just initializes which state the resource needs to
// be in for that pass, then we can determine what state the previous
// pass which uses that resource needs to move the pass too.
// 
// this can been done when determining resource lifetimes and allocating
// memory barriers.
//
// the issue is, we cannot create render passes when building render 
// graphs, purely because of how slow creating objects is.
//
// perhaps we can get around this by using pipeline image transfers
// instead of relying on the render pass transfering image.
void FrameBuilder::add_pass(RenderPass render_pass) {
    auto rpi = v::RenderPassInternal();
    
    if (render_pass.colour_image.has_value()) {
        auto config = v::pass::ColourConfig{};
        config.initial_layout = render_pass.colour_image.value()
            .get_layout();
    }
    if (render_pass.depth_image.has_value()) {

    }
}

void FrameBuilder::add_pass(ComputePass compute_pass) {
}
