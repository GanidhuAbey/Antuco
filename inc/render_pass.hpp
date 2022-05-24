#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"

#include <vector>
#include <memory>

namespace tuco {

struct DepthConfig {
    VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
};

struct ColourConfig {
    VkFormat format;
    VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

class TucoPass {
    private:
        VkRenderPass render_pass;
        bool built = false;

        bool depth_attach = false;
        VkAttachmentDescription depth_attachment{};
        VkAttachmentReference depth_ref{};
        uint32_t depth_location;

        bool colour_attach = false;
        VkAttachmentDescription colour_attachment{};
        VkAttachmentReference colour_ref{};
        uint32_t colour_location;

        std::vector<VkAttachmentDescription> attachments;

        std::shared_ptr<VkDevice> p_device;

        bool dependency = false;
        std::vector<VkSubpassDependency> dependencies;

        std::vector<VkSubpassDescription> subpasses;

    public:
        TucoPass();
        ~TucoPass();

        VkRenderPass get_api_pass();
        void build(VkDevice device, VkPipelineBindPoint bind_point);
        void add_depth(uint32_t attachment, DepthConfig config = DepthConfig{});
        void add_colour(uint32_t attachment, ColourConfig config);
        void add_dependency(std::vector<VkSubpassDependency> d);
        void create_subpass(VkPipelineBindPoint bind_point, bool colour, bool depth);


        void destroy();

    private:
        void create_render_pass(VkPipelineBindPoint bind_point);
};

}
