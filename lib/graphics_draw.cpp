//this will handle settingcl up the graphics pipeline, recording command buffers, and generally anything requiring rendering within
//the vulkan framework will end up here
#include "api_graphics.hpp"

#include "memory_allocator.hpp"
#include "data_structures.hpp"

#include "logger/interface.hpp"
#include "vulkan/vulkan_core.h"

#include "shader_text.hpp"

#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>

#include <glm/ext.hpp>

#define TIME_IT std::chrono::high_resolution_clock::now();

//TODO: this isn't a great method considering if we move around our files the whole thing will break...
const std::string SHADER_PATH = get_project_root(__FILE__) + "/shaders/";

const uint32_t MAX_FRAMES_IN_FLIGHT = 3;

using namespace tuco;

/// <summary>
///  create *p_device memory buffer to transfer data between the shadowmap texture (created by a render pass) to the shadowmap atlas
/// </summary>
void GraphicsImpl::create_shadowmap_transfer_buffer() {
    //create 4 buffers to transfer a quarter of the image at a time (this way we only have to tranfer a regular shadow map size, and later on it'll be easy
    //to multi-thread the code
    
    //because we plan to have variable shadowmaps in the future i'll make this buffer the same size as the shadow map atlas, since thats the largest
    //shadowmap that we'll have to support
    mem::BufferCreateInfo buffer_info{};
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    //maximum size stored is : region->bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize
    //imageHeight*rowLength + imageHeight*rowLength + rowLength
    buffer_info.size = (2*shadowmap_height*shadowmap_width) * 128;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.pNext = nullptr;

    for (size_t i = 0; i < SHADOW_TRANSFER_BUFFERS; i++) {
        //create all required buffers
        shadowmap_buffers[i].init(&physical_device, &*p_device, &command_pool, &buffer_info);
    } 
}

/// <summary>
/// Creates a large image to contain the depth textures of all the lights in the scene
/// </summary>
void GraphicsImpl::create_shadowmap_atlas() {
    mem::ImageCreateInfo shadow_image_info{};
    shadow_image_info.arrayLayers = 1;
    shadow_image_info.extent = VkExtent3D{ shadowmap_atlas_width, shadowmap_atlas_height, 1 };
    shadow_image_info.flags = 0;
    shadow_image_info.imageType = VK_IMAGE_TYPE_2D;
    shadow_image_info.format = VK_FORMAT_D16_UNORM;
    shadow_image_info.mipLevels = 1;
    shadow_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    shadow_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    shadow_image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    shadow_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    shadow_image_info.queueFamilyIndexCount = 1;
    shadow_image_info.pQueueFamilyIndices = &graphics_family;
    shadow_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    mem::createImage(physical_device, *p_device, &shadow_image_info, &shadowmap_atlas);

	VkImageViewUsageCreateInfo usageInfo{};
	usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
	usageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	//setup create struct for image views
	mem::ImageViewCreateInfo createInfo{};
	createInfo.pNext = &usageInfo;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = VK_FORMAT_D16_UNORM;


	//this changes the colour output of the image, currently set to standard colours
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	//layers are used for steroscopic 3d applications in which you would provide multiple images to each eye, creating a 3D effect.
	//mipmap levels are an optimization made so that lower quality textures are used when further away to save resources.
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	mem::createImageView(*p_device, createInfo, &shadowmap_atlas);

    //transfer image to depth stencil read only (or shader read only if they wont allow the first one
}

void GraphicsImpl::create_texture_sampler() {
    /* Create Sampler */
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = nullptr;
    samplerInfo.flags = 0;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(*p_device, &samplerInfo, nullptr, &texture_sampler) != VK_SUCCESS) {
        LOG("[ERROR] - createTextureSampler() : failed to create sampler object");
    }
}

void GraphicsImpl::create_shadowmap_sampler() {
    VkSamplerCreateInfo samplerInfo{};

    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = nullptr;
    samplerInfo.flags = 0;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(*p_device, &samplerInfo, nullptr, &shadowmap_sampler) != VK_SUCCESS) {
        LOG("[ERROR] - createTextureSampler() : failed to create sampler object");
    }
}

void GraphicsImpl::create_shadowpass_buffer() {  
    VkImageView image_views[1] = { shadow_pass_texture.imageView };
    create_frame_buffer(shadowpass, 1, image_views, shadowmap_width, shadowmap_height, &shadowpass_buffer);
}

void GraphicsImpl::create_swapchain_buffers() {
    size_t image_num = swapchain_images.size();
    swapchain_framebuffers.resize(image_num); 
    
    for (size_t i = 0; i < image_num; i++) {
        VkImageView image_views[2] = { swapchain_image_views[i], depth_memory.imageView };
        create_frame_buffer(render_pass, 2, image_views, swapchain_extent.width, swapchain_extent.height, &swapchain_framebuffers[i]);
    }
}

void GraphicsImpl::create_frame_buffer(VkRenderPass pass, uint32_t attachment_count, VkImageView* p_attachments, uint32_t width, uint32_t height, VkFramebuffer* frame_buffer) {
    VkFramebufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    createInfo.attachmentCount = attachment_count;
    createInfo.pAttachments = p_attachments;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    if (vkCreateFramebuffer(*p_device, &createInfo, nullptr, frame_buffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create a frame buffer");
    }
}

void GraphicsImpl::create_semaphores() {
    //create required sephamores
    VkSemaphoreCreateInfo semaphoreBegin{};
    semaphoreBegin.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(*p_device, &semaphoreBegin, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(*p_device, &semaphoreBegin, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("could not create semaphore ready signal");
        }
    }
}

void GraphicsImpl::create_fences() {
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    images_in_flight.resize(swapchain_images.size(), VK_NULL_HANDLE);
    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(*p_device, &createInfo, nullptr, &in_flight_fences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fences");
        };
    }
}

void GraphicsImpl::create_shadowpass_resources() {
    mem::ImageCreateInfo shadow_image_info{};
    shadow_image_info.arrayLayers = 1;
    shadow_image_info.extent = VkExtent3D{ shadowmap_width, shadowmap_height, 1 };
    shadow_image_info.flags = 0;
    shadow_image_info.imageType = VK_IMAGE_TYPE_2D;
    shadow_image_info.format = VK_FORMAT_D16_UNORM;
    shadow_image_info.mipLevels = 1;
    shadow_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    shadow_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    shadow_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    shadow_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    shadow_image_info.queueFamilyIndexCount = 1;
    shadow_image_info.pQueueFamilyIndices = &graphics_family;
    shadow_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //create memory for image
    mem::createImage(physical_device, *p_device, &shadow_image_info, &shadow_pass_texture);

    //transfer_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &shadow_pass_texture);

	//create image view
	VkImageViewUsageCreateInfo usageInfo{};
	usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
	usageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	//setup create struct for image views
	mem::ImageViewCreateInfo createInfo{};
	createInfo.pNext = &usageInfo;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = VK_FORMAT_D16_UNORM;


	//this changes the colour output of the image, currently set to standard colours
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	//deciding on how many layers are in the image, and if we're using any mipmap levels.
	//layers are used for steroscopic 3d applications in which you would provide multiple images to each eye, creating a 3D effect.
	//mipmap levels are an optimization made so that lower quality textures are used when further away to save resources.
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	mem::createImageView(*p_device, createInfo, &shadow_pass_texture);
}

void GraphicsImpl::create_depth_resources() {
    //create image to represent depth
    mem::ImageCreateInfo depth_image_info{};
    depth_image_info.arrayLayers = 1;
    depth_image_info.extent = VkExtent3D{ swapchain_extent.width, swapchain_extent.height, 1 };
    depth_image_info.flags = 0;
    depth_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_image_info.format = VK_FORMAT_D16_UNORM;
    depth_image_info.mipLevels = 1;
    depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depth_image_info.queueFamilyIndexCount = 1;
    depth_image_info.pQueueFamilyIndices = &graphics_family;
    depth_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //create memory for image
    mem::createImage(physical_device, *p_device, &depth_image_info, &depth_memory);

    //create image view
    VkImageViewUsageCreateInfo usageInfo{};
    usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
    usageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    //setup create struct for image views
    mem::ImageViewCreateInfo createInfo{};
    createInfo.pNext = &usageInfo;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = VK_FORMAT_D16_UNORM;


    //this changes the colour output of the image, currently set to standard colours
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    //deciding on how many layers are in the image, and if we're using any mipmap levels.
    //layers are used for steroscopic 3d applications in which you would provide multiple images to each eye, creating a 3D effect.
    //mipmap levels are an optimization made so that lower quality textures are used when further away to save resources.
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    mem::createImageView(*p_device, createInfo, &depth_memory);
}

void GraphicsImpl::create_colour_image_views() {
    swapchain_image_views.resize(swapchain_images.size());

    for (int i = 0; i < swapchain_image_views.size(); i++) {
        VkImageViewUsageCreateInfo usageInfo{};
        usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
        usageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        //setup create struct for image views
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = &usageInfo;
        createInfo.image = swapchain_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchain_format;


        //this changes the colour output of the image, currently set to standard colours
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        //deciding on how many layers are in the image, and if we're using any mipmap levels.
        //layers are used for steroscopic 3d applications in which you would provide multiple images to each eye, creating a 3D effect.
        //mipmap levels are an optimization made so that lower quality textures are used when further away to save resources.
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(*p_device, &createInfo, nullptr, &swapchain_image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("one of the image views could not be created");
        }
    }
}

void GraphicsImpl::create_shadowpass() {
    VkAttachmentDescription shadowpass_attachment{};
    shadowpass_attachment.format = VK_FORMAT_D16_UNORM;//format must be a depth/stencil format
    shadowpass_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    shadowpass_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    shadowpass_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    shadowpass_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    shadowpass_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    shadowpass_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    shadowpass_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference shadowpass_ref{};
    shadowpass_ref.attachment = 0;
    shadowpass_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &shadowpass_ref;

    std::vector<VkSubpassDependency> dependencies(2);

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; 
    
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    VkAttachmentDescription attachments[1] = { shadowpass_attachment };
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    VkSubpassDescription subpasses[1] = {subpass};
    createInfo.pSubpasses = subpasses;
    createInfo.dependencyCount = 2;
    createInfo.pDependencies = dependencies.data();


    if (vkCreateRenderPass(*p_device, &createInfo, nullptr, &shadowpass) != VK_SUCCESS) {
        throw std::runtime_error("could not create render pass");
    }
}

void GraphicsImpl::create_deffered_textures() {
    VkImageCreateInfo image_info{};
}

void GraphicsImpl::create_geometry_buffer() {

    //create_frame_buffer(&g_buffer);
}

//for deferred shading, psas which will render required information for shader computation to multiple textures
void GraphicsImpl::create_geometry_pass() {
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D16_UNORM;//format must be a depth/stencil format
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //do not need colour attachment, instead need attachment to hold image
    VkAttachmentDescription position{};
    position.format = VK_FORMAT_B8G8R8A8_SRGB;
    position.samples = VK_SAMPLE_COUNT_1_BIT;
    position.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    position.storeOp = VK_ATTACHMENT_STORE_OP_STORE;


    position.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    position.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    position.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    position.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference positionRef{};
    positionRef.attachment = 0;
    positionRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription normal{};

    normal.format = VK_FORMAT_B8G8R8A8_SRGB;
    normal.samples = VK_SAMPLE_COUNT_1_BIT;
    normal.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    normal.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    normal.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    normal.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    normal.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    normal.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference normalRef{};
    normalRef.attachment = 1;
    normalRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription positionPass{};
    positionPass.flags = 0;
    positionPass.colorAttachmentCount = 1;
    positionPass.pColorAttachments = &positionRef;

    VkSubpassDescription normalPass{};
    normalPass.flags = 0;
    normalPass.colorAttachmentCount = 1;
    normalPass.pColorAttachments = &normalRef;

    VkSubpassDescription subpasses[2] = {positionPass, normalPass};
    VkAttachmentDescription attachments[2] = { position, normal };

    VkRenderPassCreateInfo passCreate{};
    passCreate.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passCreate.pNext = nullptr;
    passCreate.flags = 0;
    passCreate.subpassCount = 2;
    passCreate.attachmentCount = 2;
    passCreate.pSubpasses = subpasses;
    passCreate.pAttachments = attachments;

    if (vkCreateRenderPass(*p_device, &passCreate, nullptr, &geometry_pass) != VK_SUCCESS) {
        LOG("could not create geometry pass");
    }
 
}

void GraphicsImpl::create_render_pass() {
    //create a depth attachment and a depth subpass
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D16_UNORM;//format must be a depth/stencil format
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //colour buffer is a buffer for the colour data at each pixel in the framebuffer, obviously important for actually drawing to screen
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchain_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    //when an image is being rendered to this is asking whether to clear everything that was on the image or store it to be readable.
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //this is too make after the rendering too screen is done makes sure the rendered contents aren't cleared and are still readable.
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    //NOTE: dont know much about stencilling, seems to be something about colouring in the image from a different layer?
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    //"The initialLayout specifies which layout the image will have before the render pass begins" - vulkan tutorial
    //"Using VK_IMAGE_LAYOUT_UNDEFINED for initialLayout means that we don't care what previous layout the image was in" - vulkan tutorial
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //the other subpass does not affect the layout of the image this subpass uses.
    //the final layout most likely means what layout the image should be transferred to at the end
    //and since we wont to present to the screen this would probably always remain as this
    //"The finalLayout specifies the layout to automatically transition to when the render pass finishes" - vulkan tutorial
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //these are subpasses you can make in the render pass to add things depending on the framebuffer in previous passes.
    //i'd assume that if you were to use these for things like post-processing you wouldn't be able to clear the image on load like
    //we do here
    VkAttachmentReference colorAttachmentRef{};
    //this refers to where the VkAttachment is and since we only have one the '0' would point to it.
    colorAttachmentRef.attachment = 0;
    //our framebuffer only has a color buffer attached to it so this layout will help optimize it
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //creating the actual subpass using the reference we created above
    VkSubpassDescription colorSubpass{};
    colorSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    colorSubpass.colorAttachmentCount = 1;
    colorSubpass.pColorAttachments = &colorAttachmentRef;
    colorSubpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 2;
    VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    VkSubpassDescription subpasses[1] = {colorSubpass};
    createInfo.pSubpasses = subpasses;
    //createInfo.dependencyCount = 1;
    //createInfo.pDependencies = &dependency;


    if (vkCreateRenderPass(*p_device, &createInfo, nullptr, &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("could not create render pass");
    }
}

void GraphicsImpl::create_light_layout() {
    /* UNIFORM BUFFER DESCRIPTOR SET */
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 1;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_layout_binding;

    if (vkCreateDescriptorSetLayout(*p_device, &layout_info, nullptr, &light_layout) != VK_SUCCESS) {
        ERR_V_MSG("COULD NOT CREATE DESCRIPTOR SET");
    }
}


void GraphicsImpl::create_materials_layout() {
    /* UNIFORM BUFFER DESCRIPTOR SET */
    VkDescriptorSetLayoutBinding mat_layout_binding{};
    mat_layout_binding.binding = 0;
    mat_layout_binding.descriptorCount = 1;
    mat_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mat_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    mat_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    layout_info.bindingCount = 1;
    layout_info.pBindings = &mat_layout_binding;

    if (vkCreateDescriptorSetLayout(*p_device, &layout_info, nullptr, &mat_layout) != VK_SUCCESS) {
        LOG("[ERROR] - could not create materials layout");
    }
}

//PURPOSE: create pool to allocate memory to.
void GraphicsImpl::create_materials_pool() {
    mem::PoolCreateInfo pool_info{};
    pool_info.pool_size = MATERIALS_POOL_SIZE;
    pool_info.set_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    mat_pool = std::make_unique<mem::Pool>(*p_device, pool_info);
}

void GraphicsImpl::create_materials_set(uint32_t mesh_count) {
    std::vector<VkDescriptorSetLayout> mat_layouts(mesh_count, mat_layout);

    VkDescriptorSetAllocateInfo allocateInfo{};

    VkDescriptorPool allocated_pool;
    size_t pool_index = mat_pool->allocate(*p_device, mesh_count);

    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = mat_pool->pools[pool_index];
    allocateInfo.descriptorSetCount = mesh_count;
    allocateInfo.pSetLayouts = mat_layouts.data();

   
    mat_sets.resize(mat_sets.size() + 1);
    mat_sets[mat_sets.size() - 1].resize(mesh_count);
    auto result = vkAllocateDescriptorSets(*p_device, &allocateInfo, mat_sets[mat_sets.size() - 1].data());
    if (result != VK_SUCCESS) {
        LOG("failed to allocate descriptor sets!");
        throw std::runtime_error("");
    } 
}


void GraphicsImpl::create_ubo_layout() {
    /* UNIFORM BUFFER DESCRIPTOR SET */
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_layout_binding;

    if (vkCreateDescriptorSetLayout(*p_device, &layout_info, nullptr, &ubo_layout) != VK_SUCCESS) {
        throw std::runtime_error("could not create ubo layout");
    }
}

void GraphicsImpl::create_ubo_pool() {
    //the previous system created both pools and descriptor sets as they were needed for new objects
    //the choices are to create new pools/descriptor sets for every object, or to reuse old descroptor sets/pools when needed...
    mem::PoolCreateInfo pool_info{};
    pool_info.pool_size = swapchain_images.size() * 50;
    pool_info.set_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    ubo_pool = std::make_unique<mem::Pool>(*p_device, pool_info);
}

void GraphicsImpl::create_ubo_set() {
    std::vector<VkDescriptorSetLayout> ubo_layouts(swapchain_images.size(), ubo_layout);

    VkDescriptorSetAllocateInfo allocateInfo{};

    VkDescriptorPool allocated_pool;
    size_t pool_index = ubo_pool->allocate(*p_device, swapchain_images.size());

    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = ubo_pool->pools[pool_index];
    allocateInfo.descriptorSetCount = swapchain_images.size();
    allocateInfo.pSetLayouts = ubo_layouts.data();

    //size_t currentSize = descriptorSets.size();
    //descriptorSets.resize(currentSize + 1);
    //descriptorSets[currentSize].resize(swapChainImages.size());

    //TODO: now we know that having a ubo copy for each swap chain image is useless, so we should change
    //      to only create one copy for every swapchain image.
    size_t current_size = ubo_sets.size();
    ubo_sets.resize(current_size + 1);
    ubo_sets[current_size].resize(swapchain_images.size());
    if (vkAllocateDescriptorSets(*p_device, &allocateInfo, ubo_sets[current_size].data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    } 
}

void GraphicsImpl::create_shadowmap_pool() {
    mem::PoolCreateInfo pool_info{};
    pool_info.pool_size = 100;
    pool_info.set_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    shadowmap_pool = std::make_unique<mem::Pool>(*p_device, pool_info);
}

void GraphicsImpl::create_texture_pool() { 
    mem::PoolCreateInfo pool_info{};
    pool_info.pool_size = swapchain_images.size() * 100;
    pool_info.set_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    texture_pool = std::make_unique<mem::Pool>(*p_device, pool_info);
}

void GraphicsImpl::create_shadowmap_set() {
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = shadowmap_pool->pools[shadowmap_pool->allocate(*p_device, 1)];
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &shadowmap_layout;

    //size_t currentSize = descriptorSets.size();
    //descriptorSets.resize(currentSize + 1);
    //descriptorSet[currentSize].resize(swapChainImages.size());

	if (vkAllocateDescriptorSets(*p_device, &allocateInfo, &shadowmap_set) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate texture sets!");
	}
}

void GraphicsImpl::create_texture_set(size_t mesh_count) {
    std::vector<VkDescriptorSetLayout> texture_layouts(mesh_count, texture_layout);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = texture_pool->pools[texture_pool->allocate(*p_device, mesh_count)];
    allocateInfo.descriptorSetCount = mesh_count;
    allocateInfo.pSetLayouts = texture_layouts.data();

    size_t current_size = texture_sets.size();
    texture_sets.resize(current_size + 1);
    texture_sets[current_size].resize(mesh_count);

    VkResult result = vkAllocateDescriptorSets(*p_device, &allocateInfo, texture_sets[current_size].data());
    if (result != VK_SUCCESS) {
        printf("[ERROR CODE %d] - could not allocate texture sets \n", result);
        throw new std::runtime_error("");
    }
}

void GraphicsImpl::create_shadowmap_layout() {
    /* SAMPLED IMAGE DESCRIPTOR SET (FOR TEXTURING) */
    VkDescriptorSetLayoutBinding texture_layout_binding{};
    texture_layout_binding.binding = 1;
    texture_layout_binding.descriptorCount = 1;
    texture_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    texture_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    layout_info.bindingCount = 1;
    layout_info.pBindings = &texture_layout_binding;

    if (vkCreateDescriptorSetLayout(*p_device, &layout_info, nullptr, &shadowmap_layout) != VK_SUCCESS) {
        throw std::runtime_error("could not create texture layout");
    }
}



void GraphicsImpl::create_texture_layout() {
    /* SAMPLED IMAGE DESCRIPTOR SET (FOR TEXTURING) */
    VkDescriptorSetLayoutBinding texture_layout_binding{};
    texture_layout_binding.binding = 0;
    texture_layout_binding.descriptorCount = 1;
    texture_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    texture_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    layout_info.bindingCount = 1;
    layout_info.pBindings = &texture_layout_binding;

    if (vkCreateDescriptorSetLayout(*p_device, &layout_info, nullptr, &texture_layout) != VK_SUCCESS) {
        throw std::runtime_error("could not create texture layout");
    }
}


VkShaderModule GraphicsImpl::create_shader_module(std::vector<uint32_t> shaderCode) {
    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);

    createInfo.pCode = shaderCode.data();

    if (vkCreateShaderModule(*p_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("could not create shader module");
    }

    return shaderModule;
}

VkPipelineShaderStageCreateInfo GraphicsImpl::fill_shader_stage_struct(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
    VkPipelineShaderStageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = stage;
    createInfo.module = shaderModule;
    createInfo.pName = "main";

    return createInfo;
}

void GraphicsImpl::create_shadowpass_pipeline() {
    VkExtent2D shadowmap_extent;
    shadowmap_extent.width = shadowmap_width;
    shadowmap_extent.height = shadowmap_height;


    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };


    std::vector<VkDescriptorSetLayout> layouts = { light_layout };

    PipelineConfig config{};
    config.vert_shader_path = SHADER_PATH + "shadow.vert";
    config.dynamic_states = dynamic_states;
    config.descriptor_layouts = layouts;
    config.screen_extent = shadowmap_extent;
    config.pass = shadowpass;
    config.subpass_index = 0;

    shadowmap_pipeline.init(*p_device, config);
}


void GraphicsImpl::create_graphics_pipeline() {
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
    };
    
    //TEX
    std::vector<VkDescriptorSetLayout> descriptor_layouts = { 

        light_layout, 
        ubo_layout, 
        texture_layout, 
        shadowmap_layout,
        mat_layout};

    std::vector<VkPushConstantRange> push_ranges;

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(LightObject) + sizeof(glm::vec4);

    push_ranges.push_back(pushRange);

    PipelineConfig config{};
    config.vert_shader_path = SHADER_PATH + "shader.vert";
    config.frag_shader_path = SHADER_PATH + "shader.frag";
    config.dynamic_states = dynamic_states;
    config.descriptor_layouts = descriptor_layouts;
    config.push_ranges = push_ranges;
    config.pass = render_pass;
    config.subpass_index = 0;
    config.screen_extent = swapchain_extent;
    config.blend_colours = true;

    graphics_pipeline.init(*p_device, config);
}


VkDescriptorBufferInfo GraphicsImpl::setup_descriptor_set_buffer(uint32_t set_size) {
    VkDeviceSize offset = uniform_buffer.allocate(set_size);

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = uniform_buffer.buffer;
    buffer_info.offset = offset;
    buffer_info.range = (VkDeviceSize)set_size;

    return buffer_info;
}

void GraphicsImpl::update_descriptor_set(VkDescriptorBufferInfo buffer_info, uint32_t dst_binding, VkDescriptorSet set) {
    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstBinding = dst_binding;
    writeInfo.dstSet = set;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeInfo.pBufferInfo = &buffer_info;
    writeInfo.dstArrayElement = 0;

    vkUpdateDescriptorSets(*p_device, 1, &writeInfo, 0, nullptr);
}

void GraphicsImpl::write_to_materials() {
    mat_offsets.resize(mat_offsets.size() + 1);
    for (size_t i = 0; i < mat_sets[mat_sets.size() - 1].size(); i++) {
        VkDescriptorBufferInfo buffer_info = setup_descriptor_set_buffer(sizeof(MaterialsObject)); 
        mat_offsets[mat_offsets.size() - 1].push_back(buffer_info.offset);
        update_descriptor_set(buffer_info, 0, mat_sets[mat_sets.size() - 1][i]);
    }
}

void GraphicsImpl::write_to_ubo() {
    VkDescriptorBufferInfo buffer_info = setup_descriptor_set_buffer(sizeof(UniformBufferObject));

    ubo_offsets.push_back(buffer_info.offset);

    for (size_t i = 0; i < swapchain_images.size(); i++) {
        update_descriptor_set(buffer_info, 0, ubo_sets[ubo_sets.size() - 1][i]);
    }
}

void GraphicsImpl::destroy_draw() {
    vkDeviceWaitIdle(*p_device);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(*p_device, in_flight_fences[i], nullptr);

        vkDestroySemaphore(*p_device, render_finished_semaphores[i], nullptr);
    }

    for (size_t i = 0; i < swapchain_images.size(); i++) {
        vkDestroyImageView(*p_device, swapchain_image_views[i], nullptr);
    }

    for (size_t i = 0; i < texture_images.size(); i++) {
        for (size_t j = 0; j < texture_images[i].size(); j++) {
            mem::destroyImage(*p_device, *texture_images[i][j]);
        }
    }

    mem::destroyImage(*p_device, shadowmap_atlas);
 
    for (auto& buffer_data : shadowmap_buffers) {
        buffer_data.destroy(*p_device);
    }
    

    mem::destroyImage(*p_device, shadow_pass_texture);

    ubo_pool->destroyPool(*p_device);
    shadowmap_pool->destroyPool(*p_device);
    texture_pool->destroyPool(*p_device);

    uniform_buffer.destroy(*p_device);
    
    vkDestroyCommandPool(*p_device, command_pool, nullptr);
        
    vkDestroyRenderPass(*p_device, shadowpass, nullptr);
    vkDestroyRenderPass(*p_device, render_pass, nullptr);

    vkDestroyDescriptorSetLayout(*p_device, ubo_layout, nullptr);
    vkDestroyDescriptorSetLayout(*p_device, light_layout, nullptr);
    vkDestroyDescriptorSetLayout(*p_device, shadowmap_layout, nullptr);
    vkDestroyDescriptorSetLayout(*p_device, texture_layout, nullptr);

    vkDestroySampler(*p_device, texture_sampler, nullptr);
    vkDestroySampler(*p_device, shadowmap_sampler, nullptr);

    for (int i = 0; i < swapchain_framebuffers.size(); i++) {
        vkDestroyFramebuffer(*p_device, swapchain_framebuffers[i], nullptr);

    }
    vkDestroyFramebuffer(*p_device, shadowpass_buffer, nullptr);

    mem::destroyImage(*p_device, depth_memory);
    vkDestroySwapchainKHR(*p_device, swapchain, nullptr);
}

VkSurfaceFormatKHR choose_best_format(std::vector<VkSurfaceFormatKHR> formats) {
    //from here we can probably choose the best format, though i don't really remember what the best formats looked like
    
    for (const auto& current_format : formats) {
        //i understand the format but forget why this colourspace is appropriate...
        if (current_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && current_format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            return current_format;
        }
    }

    //if we couldn't find the best, then we'll make due with whatever is supported
    return formats[0];
}

void GraphicsImpl::create_swapchain() {
    //query the surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities) != VK_SUCCESS) {
        LOG("[ERROR] - create_swapchain : could not query the surface capabilities");
        throw std::runtime_error("");
    }

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());

    VkSurfaceFormatKHR best_format = choose_best_format(formats);


    uint32_t min_image_num = capabilities.minImageCount;

    //why does such a case even exist????????
    if (capabilities.maxImageCount > 0 && min_image_num > capabilities.maxImageCount) {
        min_image_num = capabilities.maxImageCount;
    }

    swapchain_format = best_format.format;


    VkSwapchainCreateInfoKHR swapInfo{};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.pNext = nullptr;
    swapInfo.flags = 0;
    swapInfo.surface = surface;
    swapInfo.minImageCount = min_image_num;
    swapInfo.imageFormat = swapchain_format;
    swapInfo.imageColorSpace = best_format.colorSpace;
    swapchain_extent = capabilities.currentExtent;
    swapInfo.imageExtent = swapchain_extent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapInfo.preTransform = capabilities.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //its not in the plans to mess with the opacity of of colours, maybe in the future when glass is introduced
    swapInfo.clipped = VK_TRUE;
    swapInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(*p_device, &swapInfo, nullptr, &swapchain) != VK_SUCCESS) {
        LOG("[ERROR] - create_swapchain : could not create swapchain \n");
        throw std::runtime_error("");
    }

    //grab swapchain images
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(*p_device, swapchain, &swapchain_image_count, nullptr);
    swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(*p_device, swapchain, &swapchain_image_count, swapchain_images.data());
}



void GraphicsImpl::create_light_set(UniformBufferObject lbo) { 
    std::vector<VkDescriptorSetLayout> ubo_layouts(swapchain_images.size(), light_layout);

    VkDescriptorSetAllocateInfo allocateInfo{};

    VkDescriptorPool allocated_pool;
    size_t pool_index = ubo_pool->allocate(*p_device, swapchain_images.size());

    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = ubo_pool->pools[pool_index];
    allocateInfo.descriptorSetCount = swapchain_images.size();
    allocateInfo.pSetLayouts = ubo_layouts.data();


    //size_t currentSize = descriptorSets.size();
    //descriptorSets.resize(currentSize + 1);
    //descriptorSets[currentSize].resize(swapChainImages.size()); 
    
    size_t current_size = light_ubo.size();
    light_ubo.resize(current_size + 1);
    light_ubo[current_size].resize(swapchain_images.size());
    if (vkAllocateDescriptorSets(*p_device, &allocateInfo, light_ubo[current_size].data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    
    //write to set
    VkDeviceSize offset = uniform_buffer.allocate(sizeof(UniformBufferObject));

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = uniform_buffer.buffer;
    buffer_info.offset = offset;
    buffer_info.range = (VkDeviceSize)sizeof(UniformBufferObject);

    light_offsets.push_back(offset);

    for (size_t i = 0; i < swapchain_images.size(); i++) {
        VkWriteDescriptorSet writeInfo{};
        writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeInfo.dstBinding = 1;
        writeInfo.dstSet = light_ubo[light_ubo.size() - 1][i];
        writeInfo.descriptorCount = 1;
        writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeInfo.pBufferInfo = &buffer_info;
        writeInfo.dstArrayElement = 0;

        vkUpdateDescriptorSets(*p_device, 1, &writeInfo, 0, nullptr);
    }

    update_uniform_buffer(offset, lbo);
}

// runs a single render pass to generate a texture
/*
void GraphicsImpl::run_shadow_pass(VkCommandBuffer command_buffer) {
	//TODO: move shadow map generating code onto here to simplify create_command_buffers()
}
*/

void GraphicsImpl::free_command_buffers() {
    if (command_buffers.size() == 0) return;

    vkWaitForFences(*p_device, 1, &in_flight_fences[submitted_frame], VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(*p_device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
}

void GraphicsImpl::create_command_buffers(std::vector<GameObject*> game_objects) {
    //allocate memory for command buffer, you have to create a draw command for each image
    command_buffers.resize(swapchain_framebuffers.size());

    VkCommandBufferAllocateInfo bufferAllocate{};
    bufferAllocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocate.commandPool = command_pool;
    bufferAllocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    uint32_t imageCount = static_cast<uint32_t>(command_buffers.size());
    bufferAllocate.commandBufferCount = imageCount;

    if (vkAllocateCommandBuffers(*p_device, &bufferAllocate, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate memory for command buffers");
    }

    for (size_t i = 0; i < command_buffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(command_buffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("one of the command buffers failed to begin");
        }

		//run shadow pass for every light in the scene

        VkRenderPassBeginInfo shadowpass_info{};

        shadowpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        shadowpass_info.renderPass = shadowpass;
        shadowpass_info.framebuffer = shadowpass_buffer;
        VkRect2D renderArea{};
        renderArea.offset = VkOffset2D{ 0, 0 };
        renderArea.extent = VkExtent2D{ shadowmap_width, shadowmap_height };
        shadowpass_info.renderArea = renderArea;

        VkClearValue shadowpass_clear;
        shadowpass_clear.depthStencil = { 1.0, 0 };

        shadowpass_info.clearValueCount = 1;
        shadowpass_info.pClearValues = &shadowpass_clear;
    
        int current_shadow = 0;
        for (size_t y = 0; y < glm::sqrt(MAX_SHADOW_CASTERS); y++) {
            for (size_t x = 0; x < glm::sqrt(MAX_SHADOW_CASTERS); x++) {
                if (shadow_caster_indices.size() > current_shadow) { 
                    //there is at least one light that is casting a shadow here
                    vkCmdBeginRenderPass(command_buffers[i], &shadowpass_info, VK_SUBPASS_CONTENTS_INLINE);
                    //do stuff...
                    vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowmap_pipeline.get_api_pipeline());

                    VkViewport shadowpass_port = {};
                    shadowpass_port.x = 0;
                    shadowpass_port.y = 0;
                    shadowpass_port.width = (float)shadowmap_width;
                    shadowpass_port.height = (float)shadowmap_height;
                    shadowpass_port.minDepth = 0.0;
                    shadowpass_port.maxDepth = 1.0;
                    vkCmdSetViewport(command_buffers[i], 0, 1, &shadowpass_port);

                    VkRect2D shadowpass_scissor{};
                    shadowpass_scissor.offset = { 0, 0 };
                    shadowpass_scissor.extent.width = shadowmap_width;
                    shadowpass_scissor.extent.height = shadowmap_height;
                    vkCmdSetScissor(command_buffers[i], 0, 1, &shadowpass_scissor);

                    //time for the draw calls
                    const VkDeviceSize offsets[] = { 0, offsetof(Vertex, normal), offsetof(Vertex, tex_coord) };
                    vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffer.buffer, offsets);
                    vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

                    //vkCmdSetDepthBias(command_buffers[i], 5.0f, 0.0f, 1.75f);

                    //universal to every object so i can push the light constants before the for loop
                    //vkCmdPushConstants(shadowpass_render, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(light), &light);
                    //draw first object (cube)
                    uint32_t total_indexes = 0;
                    uint32_t total_vertices = 0;

                    uint32_t* number;
                    
                    for (size_t j = 0; j < game_objects.size(); j++) {
                        if (!game_objects[j]->update) {
                            for (size_t k = 0; k < game_objects[j]->object_model.model_meshes.size(); k++) {
                                const Mesh* mesh_data = game_objects[j]->object_model.model_meshes[k];
                                uint32_t index_count = static_cast<uint32_t>(mesh_data->indices.size());
                                uint32_t vertex_count = static_cast<uint32_t>(mesh_data->vertices.size());

                                //we're kinda phasing object colours out with the introduction of textures, so i'm probably not gonna need to push this
                                //vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(light), sizeof(pfcs[i]), &pfcs[i]);

                                VkDescriptorSet descriptors[1] = { light_ubo[j][i] };
                                vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowmap_pipeline.get_api_layout(), 0, 1, descriptors, 0, nullptr);
                                vkCmdDrawIndexed(command_buffers[i], index_count, 1, total_indexes, total_vertices, static_cast<uint32_t>(0));

                                total_indexes += index_count;
                                total_vertices += vertex_count;
                            }
                        }
                    }
                    vkCmdEndRenderPass(command_buffers[i]);

                    transfer_image_layout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, shadow_pass_texture.image, VK_IMAGE_ASPECT_DEPTH_BIT, command_buffers[i]);


                    //take depth texture calculated in first render pass from DEPTH_STENCIL_READ_ONLY -> TRANSFER_SRC       
                    copy_image_to_buffer(shadowmap_buffers[0].buffer, shadow_pass_texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, command_buffers[i]);
                    transfer_image_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, shadow_pass_texture.image, VK_IMAGE_ASPECT_DEPTH_BIT, command_buffers[i]);

                    //transfer the image data from the intermediary buffer into the shadow map
                    transfer_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, shadowmap_atlas.image, VK_IMAGE_ASPECT_DEPTH_BIT, command_buffers[i]);
                    VkOffset3D image_offset = {static_cast<int32_t>(x*SHADOWMAP_SIZE), static_cast<int32_t>(y*SHADOWMAP_SIZE), 0}; 
                    copy_buffer_to_image(shadowmap_buffers[0].buffer, shadowmap_atlas, image_offset, VK_IMAGE_ASPECT_DEPTH_BIT, SHADOWMAP_SIZE, SHADOWMAP_SIZE, command_buffers[i]);
                    transfer_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shadowmap_atlas.image, VK_IMAGE_ASPECT_DEPTH_BIT, command_buffers[i]);
                }
                current_shadow++;
            }			
		}
        
        /*
        {
            VkRenderPassBeginInfo g_pass{};
            g_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            g_pass.renderPass = geometry_pass;

        }
        */
    
		VkRenderPassBeginInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderInfo.renderPass = render_pass;
		renderInfo.framebuffer = swapchain_framebuffers[i];
		renderArea.offset = VkOffset2D{ 0, 0 };
		renderArea.extent = swapchain_extent;
		renderInfo.renderArea = renderArea;

		const size_t size_of_array = 3;
		std::vector<VkClearValue> clear_values;

		VkClearValue color_clear;
		color_clear.color = { 0.0f, 0.0f, 0.0f, 1.0 }; 

		VkClearValue depth_clear;
		depth_clear.depthStencil = {1.0, 0};

		clear_values.push_back(color_clear);
		clear_values.push_back(depth_clear);
		
		renderInfo.clearValueCount = static_cast<uint32_t>(clear_values.size());
		renderInfo.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffers[i], &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
		//run first subpass here?
		//the question is what does the first subpass do?

		//now after the first pass is complete we move on to the next subpass
		//vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);

		//add commands to command buffer
		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.get_api_pipeline());

		VkViewport newViewport{};
		newViewport.x = 0;
		newViewport.y = 0;
		newViewport.width = (float)swapchain_extent.width;
		newViewport.height = (float)swapchain_extent.height;
        newViewport.minDepth = 0.0;
		newViewport.maxDepth = 1.0;
		vkCmdSetViewport(command_buffers[i], 0, 1, &newViewport);

		VkRect2D newScissor{};
		newScissor.offset = { 0, 0 };
		newScissor.extent = swapchain_extent;
		vkCmdSetScissor(command_buffers[i], 0, 1, &newScissor);

		//time for the draw calls
		const VkDeviceSize offset[] = { 0, offsetof(Vertex, normal), offsetof(Vertex, tex_coord) };
		vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffer.buffer, offset);
		vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		//universal to every object so i can push the light constants before the for loop
		//convert to light_object;
		LightObject light{};
		light.position = light_data[0]->position;
		light.direction = light_data[0]->target;
		light.color = light_data[0]->color;
        light.light_count = glm::vec4(MAX_SHADOW_CASTERS, 1, 1, 1); //NOTE: will only pass into the shader with a vec4, don't ask me why it seems glsl really really values constant spacing
        
		vkCmdPushConstants(command_buffers[i], graphics_pipeline.get_api_layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(light), &light);
        vkCmdPushConstants(command_buffers[i], graphics_pipeline.get_api_layout(), VK_SHADER_STAGE_VERTEX_BIT, sizeof(light), sizeof(glm::vec4), &camera_pos); 
		//draw first object (cube)
		uint32_t total_indexes = 0;
		uint32_t total_vertices = 0;

        for (size_t j = 0; j < game_objects.size(); j++) {
            if (!game_objects[j]->update) {
                for (size_t k = 0; k < game_objects[j]->object_model.model_meshes.size(); k++) {
                    Mesh* mesh_data = game_objects[j]->object_model.model_meshes[k];
                    uint32_t index_count = static_cast<uint32_t>(mesh_data->indices.size());
                    uint32_t vertex_count = static_cast<uint32_t>(mesh_data->vertices.size());


                    //we're kinda phasing object colours out with the introduction of textures, so i'm probably not gonna need to push this
                    //vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(light), sizeof(pfcs[i]), &pfcs[i]);
                   
                    //PROBLEM IS THAT texture set is not updated when the model/mesh we're dealing with has not texture
                    VkDescriptorSet descriptors[5] = { light_ubo[j][i], ubo_sets[j][i], texture_sets[j][k], shadowmap_set, mat_sets[j][k] };
                    vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.get_api_layout(), 0, 5, descriptors, 0, nullptr); 

                    vkCmdDrawIndexed(command_buffers[i], index_count, 1, total_indexes, total_vertices, static_cast<uint32_t>(0));

                    total_indexes += index_count;
                    total_vertices += vertex_count;
                }
            }
        }

        //vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(6), 1, 36, 0, 0);
        //vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(command_buffers[i]);
        //switch image back to depth stencil layout for the next render pass

        //end commands to go to execute stage
        if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("could not end command buffer");
        }
    }
}

void GraphicsImpl::create_uniform_buffer() {
    mem::BufferCreateInfo buffer_info{};
    buffer_info.size = (VkDeviceSize)BUFFER_SIZE;
    buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uniform_buffer.init(physical_device, *p_device, &buffer_info);
}

void GraphicsImpl::create_vertex_buffer() {
    mem::BufferCreateInfo buffer_info{};
    buffer_info.size = (VkDeviceSize)BUFFER_SIZE;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vertex_buffer.init(&physical_device, &*p_device, &command_pool, &buffer_info);
}

void GraphicsImpl::create_index_buffer() {
    mem::BufferCreateInfo buffer_info{};
    buffer_info.size = (VkDeviceSize)BUFFER_SIZE;
    buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    index_buffer.init(&physical_device, &*p_device, &command_pool, &buffer_info);
}


//REQUIRES: indices_data is a list of indices corresponding to the vertices provided to the vertex buffer
//MODIFIES: this
//EFFECTS: allocates memory and maps indices_data to index buffer, returns location in memory as int or '-1' if
//         new data was mapped.
int32_t GraphicsImpl::update_index_buffer(std::vector<uint32_t> indices_data) {
    if (check_data(indices_data.size() * sizeof(uint32_t))) {
        return -1;
    }
    uint32_t mem_location = index_buffer.map(indices_data.size() * sizeof(uint32_t), indices_data.data()); 
    
    return mem_location;
}

//helper function to check if data exists or print warning if it doesnt
bool GraphicsImpl::check_data(size_t data_size) {
    if (data_size == 0) {
        printf("[WARNING] - attempted to allocate data of size 0, preserving previous state... \n");
        return true;
    }

    return false;
}

//MODIFIES: this
//PURPOSE: adds given vertex_data to vertex buffer
//RETURNS: memory location of where data was added or '-1' if no new data was mapped
int32_t GraphicsImpl::update_vertex_buffer(std::vector<Vertex> vertex_data) {
    if (check_data(vertex_data.size() * sizeof(Vertex))) return -1;

    uint32_t mem_location = vertex_buffer.map(vertex_data.size() * sizeof(Vertex), vertex_data.data());
    return mem_location;
}

void GraphicsImpl::copy_buffer(mem::Memory src_buffer, mem::Memory dst_buffer, VkDeviceSize dst_offset, VkDeviceSize data_size) {
    VkCommandBuffer transferBuffer = begin_command_buffer();

    //transfer between buffers
    VkBufferCopy copyData{};
    copyData.srcOffset = 0;
    copyData.dstOffset = dst_offset;
    copyData.size = data_size;

    vkCmdCopyBuffer(transferBuffer,
        src_buffer.buffer,
        dst_buffer.buffer,
        1,
        &copyData
    );

    //destroy transfer buffer, shouldnt need it after copying the data.
    end_command_buffer(transferBuffer);
}

VkCommandBuffer GraphicsImpl::begin_command_buffer() {
    //create command buffer
    VkCommandBuffer transferBuffer;

    VkCommandBufferAllocateInfo bufferAllocate{};
    bufferAllocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocate.commandPool = command_pool;
    bufferAllocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocate.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(*p_device, &bufferAllocate, &transferBuffer) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate memory for transfer buffer");
    }

    //begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(transferBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("one of the command buffers failed to begin");
    }

    return transferBuffer;
}


void GraphicsImpl::end_command_buffer(VkCommandBuffer commandBuffer) {
    //end command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create succesfully end transfer buffer");
    };

    //destroy transfer buffer, shouldnt need it after copying the data.
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(*p_device, command_pool, 1, &commandBuffer);
}

void GraphicsImpl::update_light_buffer(VkDeviceSize memory_offset, LightBufferObject lbo) {
    uniform_buffer.write(*p_device, memory_offset, sizeof(lbo), &lbo);
}

void GraphicsImpl::update_materials(VkDeviceSize memory_offset, MaterialsObject mat) {
    uniform_buffer.write(*p_device, memory_offset, sizeof(mat), &mat);
}

void GraphicsImpl::update_uniform_buffer(VkDeviceSize memory_offset, UniformBufferObject ubo) {
    //overwrite memory
    uniform_buffer.write(*p_device, memory_offset, sizeof(ubo), &ubo);
}

/// <summary>
/// When called, this function will destroy all objects associated with the swapchain.
/// </summary>
void GraphicsImpl::cleanup_swapchain() {
    vkDeviceWaitIdle(*p_device);

    for (const auto& frame_buffer : swapchain_framebuffers) {
        vkDestroyFramebuffer(*p_device, frame_buffer, nullptr);
    }

    for (size_t i = 0; i < swapchain_images.size(); i++) {
        vkDestroyImageView(*p_device, swapchain_image_views[i], nullptr);
    }
 
    mem::destroyImage(*p_device, depth_memory);
    vkDestroyRenderPass(*p_device, render_pass, nullptr);
    vkDestroySwapchainKHR(*p_device, swapchain, nullptr);
}

/// <summary>
/// When called, this function create all necesarry objects for the swapchain
/// </summary>
void GraphicsImpl::recreate_swapchain() {
    create_swapchain();
    create_depth_resources();
    create_colour_image_views();
    create_render_pass();
    create_swapchain_buffers();
        
    create_command_buffers(*recent_objects);
}

//TODO: need to make better use of cpu-cores
void GraphicsImpl::draw_frame() {
    //make sure that the current frame thats being drawn in parallel is available
    vkWaitForFences(*p_device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
    //allocate memory to store next image
    uint32_t nextImage;

    VkResult result = vkAcquireNextImageKHR(*p_device, swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &nextImage);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("could not aquire image from swapchain");
    }

    if (images_in_flight[nextImage] != VK_NULL_HANDLE) {
        vkWaitForFences(*p_device, 1, &images_in_flight[nextImage], VK_TRUE, UINT64_MAX);
        //imagesInFlight[nextImage] = VK_NULL_HANDLE;
    }
    // Mark the image as now being in use by this frame
    images_in_flight[nextImage] = in_flight_fences[current_frame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //add appropriate command buffer
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore waitSemaphores[] = {image_available_semaphores[current_frame]};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffers[nextImage];
    //signal to send when this command buffer has executed.
    VkSemaphore signalSemaphores[] = {render_finished_semaphores[current_frame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(*p_device, 1, &in_flight_fences[current_frame]);

    if (vkQueueSubmit(graphics_queue, 1, &submitInfo, in_flight_fences[current_frame]) !=  VK_SUCCESS) {
        throw std::runtime_error("could not submit command buffer to queue");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &nextImage;
    presentInfo.pResults = nullptr;

    VkResult presentResult = vkQueuePresentKHR(present_queue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        cleanup_swapchain();
        recreate_swapchain();
    }
    
    submitted_frame = current_frame;
    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

//TODO: a bug is causing vulkan to spit validation errors because i'm checking whether the 'command_buffer' parameter has a value...
//      but 'command_buffer' is unused so it never has a value and therefore the functions should behave as if 'command_buffer' doesn't exist
//      but clearly that isn't happening.
//
//      need to figure it out
void GraphicsImpl::transfer_image_layout(VkImageLayout initial_layout, VkImageLayout output_layout, VkImage image, VkImageAspectFlagBits aspect_mask, std::optional<VkCommandBuffer> command_buffer) {
    //begin command buffer
    bool delete_buffer = false;
    if (!command_buffer.has_value()) {
        command_buffer = begin_command_buffer();
        delete_buffer = true;
    }


    //transfer image layout
    VkImageMemoryBarrier imageTransfer{};
    imageTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageTransfer.pNext = nullptr;
    imageTransfer.oldLayout = initial_layout;
    imageTransfer.newLayout = output_layout;
    imageTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageTransfer.image = image;
    imageTransfer.subresourceRange.aspectMask = aspect_mask;
    imageTransfer.subresourceRange.baseMipLevel = 0;
    imageTransfer.subresourceRange.levelCount = 1;
    imageTransfer.subresourceRange.baseArrayLayer = 0;
    imageTransfer.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    if (output_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        imageTransfer.srcAccessMask = 0; // this basically means none or doesnt matter
        imageTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_SHADER_READ_BIT; 
        imageTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;


        sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        imageTransfer.srcAccessMask = 0;
        imageTransfer.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        imageTransfer.srcAccessMask = 0;
        imageTransfer.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        imageTransfer.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        imageTransfer.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {  
        imageTransfer.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        imageTransfer.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    vkCmdPipelineBarrier(
        command_buffer.value(),
        sourceStage,
        destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageTransfer
    );

    //end command buffer
    if (delete_buffer) {
        end_command_buffer(command_buffer.value());
    }
}

/// <summary>
///   Copies the entire contents of an image onto a particular place in a buffer
/// </summary>
/// <param name="buffer"> buffer the image data will be copied to </param>
/// <param name="image"> the image that will be transfered </param>
/// <param name="image_layout"> the layout of the iamge </param>
/// <param name="image_aspect"> the aspect of the image </param>
/// <param name="dst_offset"> the offset of the buffer</param>
void GraphicsImpl::copy_image_to_buffer(VkBuffer buffer, mem::Memory image, VkImageLayout image_layout, VkImageAspectFlagBits image_aspect, VkDeviceSize dst_offset, std::optional<VkCommandBuffer> command_buffer)  {
    bool delete_buffer = false; 
    if (!command_buffer.has_value()) { 
        command_buffer = begin_command_buffer();
        delete_buffer = true;
    }

    //we won't be dealing with mipmap levels for a while i think so i can change this then
    VkImageSubresourceLayers image_layers{};
    image_layers.aspectMask = image_aspect;
    image_layers.mipLevel = 0;
    image_layers.layerCount = 1;
    image_layers.baseArrayLayer = 0;

    VkBufferImageCopy copy_data{};
    copy_data.bufferOffset = dst_offset;
    copy_data.bufferImageHeight = 0.0f; //ignore
    copy_data.bufferRowLength = 0.0f;
    copy_data.imageSubresource = image_layers;
    copy_data.imageOffset = VkOffset3D{ 0, 0, 0 };
    copy_data.imageExtent = VkExtent3D{ image.imageDimensions.width, image.imageDimensions.height, 1 };

    //copy image to buffer
    //TODO: contain this various image data within the memory object so the user doesnt have to keep track of it
    vkCmdCopyImageToBuffer(command_buffer.value(), image.image, image_layout, buffer, 1, &copy_data);

    if (delete_buffer) { 
        end_command_buffer(command_buffer.value());
    }
}

///
/// transfers data from a buffer to an image
/// PARAMETERS
///     - mem::Memory buffer : the source buffer that the data will be transferred from
///     - mem::Memory image : the image that the data will be transferred to
///     - VkExtent3D image_offset : which part of the image the buffer data will be mapped to
///     - uint32_t image_width : width of the image
///     - uint32_t image_height : height of the image
/// RETURNS - VOID
void GraphicsImpl::copy_buffer_to_image(VkBuffer buffer, mem::Memory image, VkOffset3D image_offset, VkImageAspectFlagBits aspect_mask, uint32_t image_width, uint32_t image_height, std::optional<VkCommandBuffer> command_buffer) {
    //create command buffer
    bool delete_buffer = false;
    if (!command_buffer.has_value()) {
        command_buffer = begin_command_buffer();
        delete_buffer = true;
    }

    VkImageSubresourceLayers imageSub{};
    imageSub.aspectMask = aspect_mask;
    imageSub.mipLevel = 0;
    imageSub.baseArrayLayer = 0;
    imageSub.layerCount = 1;

    //note for future me: this isn't about the size of destination image, its about the size of image in the buffer that your copying over.
    VkExtent3D imageExtent = {
        image_width,
        image_height,
        1,
    };

    VkBufferImageCopy bufferCopy{};
    bufferCopy.bufferOffset = 0;
    bufferCopy.bufferRowLength = 0;
    bufferCopy.bufferImageHeight = 0;
    bufferCopy.imageSubresource = imageSub;
    bufferCopy.imageOffset = image_offset;
    bufferCopy.imageExtent = imageExtent;

    vkCmdCopyBufferToImage(command_buffer.value(),
        buffer,
        image.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bufferCopy
    );

    //destroy transfer buffer, shouldnt need it after copying the data.
    if (delete_buffer) {
        end_command_buffer(command_buffer.value());
    }
}

void GraphicsImpl::write_to_shadowmap_set() {
    VkDescriptorImageInfo imageInfo;
    imageInfo.sampler = shadowmap_sampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = shadowmap_atlas.imageView;

    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstBinding = 1;
    writeInfo.dstSet = shadowmap_set;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeInfo.pImageInfo = &imageInfo;
    writeInfo.dstArrayElement = 0;

    vkUpdateDescriptorSets(*p_device, 1, &writeInfo, 0, nullptr);
    
    //wonder if this is a fine time to transfer the image from undefined to shader
    //TODO: move this somewhere else (into its own function if you absolutely have to)
    transfer_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shadowmap_atlas.image, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void GraphicsImpl::write_to_texture_set(VkDescriptorSet texture_set, mem::Memory* image) {
    VkDescriptorImageInfo imageInfo;
    imageInfo.sampler = texture_sampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image->imageView;

    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstBinding = 0;
    writeInfo.dstSet = texture_set;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeInfo.pImageInfo = &imageInfo;
    writeInfo.dstArrayElement = 0;

    vkUpdateDescriptorSets(*p_device, 1, &writeInfo, 0, nullptr);
}

void GraphicsImpl::create_empty_image(size_t object, size_t texture_set) {
    //CREATE A WHITE IMAGE TO SEND TO THE SHADER THIS WAY WE CAN ALSO ELIMINATE THE IF STATEMENTS 
    
    //create vector of white pixels... and then map that to image?
    mem::Memory* new_texture_image = new mem::Memory{};

    mem::ImageCreateInfo imageInfo{};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    VkExtent3D extent{};
    extent.width = 1;
    extent.height = 1;
    extent.depth = 1; //this depth might be key to blending the textures...
    imageInfo.extent = extent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 1;
    imageInfo.pQueueFamilyIndices = &graphics_family;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    mem::createImage(physical_device, *p_device, &imageInfo, new_texture_image);

    //transfer the image again to a more optimal layout for texture sampling?
    transfer_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, new_texture_image->image, VK_IMAGE_ASPECT_COLOR_BIT);

    //create image view for image
    mem::ImageViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    mem::createImageView(*p_device, viewInfo, new_texture_image);

    //save texture image to mesh
    mem::ImageSize image_dimensions{};
    image_dimensions.width = 1;
    image_dimensions.height = 1;

    texture_images.resize(texture_images.size() + 1);
    texture_images[texture_images.size() - 1].push_back(new_texture_image);

    //write to texture set
    write_to_texture_set(texture_sets[object][texture_set], new_texture_image);

}


void GraphicsImpl::create_texture_image(aiString texturePath, size_t object, size_t texture_set) {
    int imageWidth, imageHeight, imageChannels;
    //this is a char... why is that?
    stbi_uc *pixels = stbi_load(texturePath.data, &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);

    VkDeviceSize dataSize = 4 * (imageWidth * imageHeight);
    mem::BufferCreateInfo textureBufferInfo{};
    textureBufferInfo.size = dataSize;
    textureBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    textureBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    textureBufferInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    textureBufferInfo.queueFamilyIndexCount = 1;
    textureBufferInfo.pQueueFamilyIndices = &graphics_family;

    //TODO: make buffer at runtime specifically for transfer commands
    mem::Memory newBuffer;
    mem::createBuffer(physical_device, *p_device, &textureBufferInfo, &newBuffer);
    
    mem::allocateMemory(dataSize, &newBuffer);
    mem::mapMemory(*p_device, dataSize, &newBuffer, pixels);

    //use size of loaded image to create VkImage
    mem::Memory* new_texture_image = new mem::Memory{};

    mem::ImageCreateInfo imageInfo{};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    VkExtent3D extent{};
    extent.width = static_cast<uint32_t>(imageWidth);
    extent.height = static_cast<uint32_t>(imageHeight);
    extent.depth = 1; //this depth might be key to blending the textures...
    imageInfo.extent = extent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 1;
    imageInfo.pQueueFamilyIndices = &graphics_family;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    mem::createImage(physical_device, *p_device, &imageInfo, new_texture_image);

    //mem::maAllocateMemory(dataSize, &newTextureImage);
    //transfer the image to appropriate layout for copying
    transfer_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, new_texture_image->image, VK_IMAGE_ASPECT_COLOR_BIT);
    VkOffset3D image_offset = {0, 0, 0};
    copy_buffer_to_image(newBuffer.buffer, *new_texture_image, image_offset, VK_IMAGE_ASPECT_COLOR_BIT, new_texture_image->imageDimensions.width, new_texture_image->imageDimensions.height);
    //transfer the image again to a more optimal layout for texture sampling?
    transfer_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, new_texture_image->image, VK_IMAGE_ASPECT_COLOR_BIT);

    //create image view for image
    mem::ImageViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    mem::createImageView(*p_device, viewInfo, new_texture_image);

    mem::destroyBuffer(*p_device, newBuffer);

    //save texture image to mesh
    mem::ImageSize image_dimensions{};
    image_dimensions.width = static_cast<uint32_t>(imageWidth);
    image_dimensions.height = static_cast<uint32_t>(imageHeight);

    texture_images.resize(texture_images.size() + 1);
    texture_images[texture_images.size() - 1].push_back(new_texture_image);

    //write to texture set
    write_to_texture_set(texture_sets[object][texture_set], new_texture_image);

    //free image
    stbi_image_free(pixels);
}
