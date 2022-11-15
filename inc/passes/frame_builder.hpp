#pragma once

#include <vector>
#include <memory>

#include <passes/render_pass.hpp>
#include <passes/compute_pass.hpp>

#include <passes/frame_resources.hpp>

#include <vulkan_wrapper/render_pass_internal.hpp>
#include <vulkan_wrapper/swapchain.hpp>
#include <vulkan_wrapper/image.hpp>

using namespace builder;


namespace pass {

// Take our defined passes and convert thme into commands which can be
// recorded to the command buffer and submitted.
class FrameBuilder {
private:
    std::vector<RenderPass> render_passes;

    struct InternalImageResource {
        ImageResource data;
        Pass pass_origin;
        Usage use;
    };

    std::unordered_map<uint32_t, Image> image_resources;

    v::Swapchain swapchain;

public:
    FrameBuilder(v::Swapchain& swapchain) {
        FrameBuilder::swapchain = swapchain;
    }
    ~FrameBuilder() = default;

    static std::unique_ptr<FrameBuilder> p_frame_builder;
    static FrameBuilder* get();

    void initialize();
    void setup_frame();
    void execute_frame();

    // Safer option is likely to define pass order from within the passes.
    // Stating what comes before and what comes after should be enough
    void add_pass(RenderPass pass);
    void add_pass(ComputePass pass); 

    // create the required resource
    Image* find_or_create(ImageResource resource);

    Dimensions get_window_dimensions() {
        auto extent = swapchain.get_extent();

        Dimensions dim(extent.width, extent.height, 1);

        return dim;
    }
};

}
