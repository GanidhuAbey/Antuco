#pragma once

#include <vector>
#include <memory>

#include <passes/render_pass.hpp>
#include <passes/compute_pass.hpp>

#include <passes/frame_resources.hpp>

#include <vulkan_wrapper/render_pass_internal.hpp>

using namespace builder;


namespace pass {

// Take our defined passes and convert thme into commands which can be
// recorded to the command buffer and submitted.
class FrameBuilder {
private:
    std::vector<v::RenderPassInternal> render_passes;

    struct InternalImageResource {
        ImageResource data;
        Pass pass_origin;
        Usage use;
    };

    std::vector<InternalImageResource> image_resource;

public:
    FrameBuilder() = default;
    ~FrameBuilder() = default;

    static std::unique_ptr<FrameBuilder> p_frame_builder;
    static FrameBuilder* get();

    void build_frame();

    // Safer option is likely to define pass order from within the passes.
    // Stating what comes before and what comes after should be enough
    void add_pass(RenderPass pass);
    void add_pass(ComputePass pass); 

    // create the required resource
    void create_image_resource(ImageResource resource, Pass pass);
};

}
