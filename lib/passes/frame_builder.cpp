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

void FrameBuilder::initialize() {
    for (auto& render_pass : render_passes) {
        render_pass.initialize();
    }

    for (const auto& image_data : image_resources) {
        
    }
}

void FrameBuilder::setup_frame() {
}

void FrameBuilder::execute_frame() {}

Image* FrameBuilder::find_or_create(ImageResource resource) {
    if (resource.type == NO_IMAGE) {
        return nullptr;
    }

    // search for resource
    auto value = image_resources.find(resource.id);
    if (value != image_resources.end()) {
        return &value->second;
    }

    // create and add resource, if resource not found.
    Image image(resource);

    image_resources[resource.id] = std::move(image);

    auto value = image_resources.find(resource.id);
    
    return &value->second;
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
    
    render_passes.push_back(render_pass);
}

void FrameBuilder::add_pass(ComputePass compute_pass) {
}
