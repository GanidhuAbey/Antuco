#pragma once

#include <vulkan/vulkan.hpp>

#include "vulkan_wrapper/device.hpp"

#include <vector>
#include <memory>

namespace tuco {

struct DepthConfig {
    VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
};

struct ColourConfig {
    vk::Format format;
    VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    vk::ImageLayout final_layout = vk::ImageLayout::ePresentSrcKHR;
};

class TucoPass {
    private:
        v::Device* api_device;

        VkRenderPass render_pass;

        bool depth_attach = false;
        VkAttachmentDescription depth_attachment{};
        VkAttachmentReference depth_ref{};
        uint32_t depth_location;

        bool colour_attach = false;
        vk::AttachmentDescription colour_attachment{};
        VkAttachmentReference colour_ref{};
        uint32_t colour_location;

        std::vector<vk::AttachmentDescription> attachments;

        bool dependency = false;
        std::vector<vk::SubpassDependency> dependencies;

        std::vector<vk::SubpassDescription> subpasses;

    public:
        VkRenderPass get_api_pass();
        void build(v::Device& device, VkPipelineBindPoint bind_point);
        void add_depth(uint32_t attachment, DepthConfig config = DepthConfig{});
        void add_colour(uint32_t attachment, ColourConfig config);
        void add_dependency(std::vector<vk::SubpassDependency> d);
        void create_subpass(VkPipelineBindPoint bind_point, bool colour, bool depth);


        void destroy();

    private:
        void create_render_pass(VkPipelineBindPoint bind_point);
};

}
