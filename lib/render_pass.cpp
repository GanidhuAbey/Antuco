#include "render_pass.hpp"

#include "logger/interface.hpp"

#include <cstdio>
#include <memory>
#include <stdexcept>

using namespace tuco;


void TucoPass::destroy() {
    api_device->get().destroyRenderPass(render_pass);
}

void TucoPass::build(v::Device& device, VkPipelineBindPoint bind_point) {
    api_device = &device;
    
    create_render_pass(bind_point);
}

VkRenderPass TucoPass::get_api_pass() {
    return render_pass;
}

void TucoPass::add_depth(uint32_t attachment, DepthConfig config) {
    depth_attach = true;

    depth_location = attachment;

    depth_attachment.format = VK_FORMAT_D16_UNORM;//format must be a depth/stencil format
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    depth_attachment.initialLayout = config.initial_layout;
    depth_attachment.finalLayout = config.final_layout;

    depth_ref.attachment = depth_location;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachments.push_back(depth_attachment); 
}

void TucoPass::add_colour(uint32_t attachment, ColourConfig config) {
    colour_attach = true;

    colour_location = attachment;

    colour_ref.attachment = attachment;
    colour_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    colour_attachment = vk::AttachmentDescription(
            {}, 
            config.format, 
            vk::SampleCountFlagBits::e1, 
            vk::AttachmentLoadOp::eClear, 
            vk::AttachmentStoreOp::eStore, 
            vk::AttachmentLoadOp::eDontCare, 
            vk::AttachmentStoreOp::eDontCare, 
            vk::ImageLayout::eUndefined, 
            config.final_layout
        );

    attachments.push_back(colour_attachment);
}

void TucoPass::add_dependency(std::vector<vk::SubpassDependency> d) {
    dependencies = d;
    dependency = true; 
}

void TucoPass::create_subpass(VkPipelineBindPoint bind_point, bool colour, bool depth) {
    if (colour != colour_attach || depth != depth_attach) {
        ERR("attempted to enable colour or depth with no attachments created");
        return;
    } 
    
    VkSubpassDescription subpass{};
    subpass.flags = 0;
    subpass.pipelineBindPoint = bind_point;
    subpass.colorAttachmentCount = 0; 
    if (colour) {
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colour_ref;
    } if (depth) {
        subpass.pDepthStencilAttachment = &depth_ref;
    }
    subpasses.push_back(subpass);
}

void TucoPass::create_render_pass(VkPipelineBindPoint bind_point) {
    if (subpasses.size() == 0) {
        ERR("at least one subpass is required to create render pass");
    }

    auto info = vk::RenderPassCreateInfo(
            {},
            static_cast<uint32_t>(attachments.size()),
            attachments.data(),
            static_cast<uint32_t>(subpasses.size()),
            subpasses.data(),
            static_cast<uint32_t>(dependencies.size()),
            dependencies.data()
        );
    
    render_pass = api_device->get().createRenderPass(info); 
}
