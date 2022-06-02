//this will handle settingcl up the graphics pipeline, recording command buffers, and generally anything requiring rendering within
//the vulkan framework will end up here
#include "api_graphics.hpp"

#include "memory_allocator.hpp"
#include "data_structures.hpp"

#include "logger/interface.hpp"
#include "vulkan/vulkan_core.h"

#include "shader_text.hpp"

#include <optional>
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
        shadowmap_buffers[i].init(p_physical_device.get(), &*p_device, &command_pool, &buffer_info);
    } 
}


void GraphicsImpl::create_depth_pipeline() {
    PipelineConfig config{};
    config.vert_shader_path = SHADER_PATH + "shader.vert";
    config.blend_colours = false;
    config.descriptor_layouts = {light_layout, ubo_layout};
    
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    config.dynamic_states = dynamic_states;
    //config.pass
}

/// <summary>
/// Creates a large image to contain the depth textures of all the lights in the scene
/// </summary>
void GraphicsImpl::create_shadowmap_atlas() {
    mem::ImageCreateInfo shadow_image_info{};
    shadow_image_info.extent = VkExtent3D{ shadowmap_atlas_width, shadowmap_atlas_height, 1 };
    shadow_image_info.format = VK_FORMAT_D16_UNORM;
    shadow_image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    shadow_image_info.queueFamilyIndexCount = 1;
    shadow_image_info.pQueueFamilyIndices = &graphics_family;

	VkImageViewUsageCreateInfo usageInfo{};
	usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
	usageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	//setup create struct for image views
	mem::ImageViewCreateInfo createInfo{};
	createInfo.pNext = &usageInfo; 
    createInfo.aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

    mem::ImageData data{};
    data.name = "shadowmap";
    data.image_info = shadow_image_info;
    data.image_view_info = createInfo;

    shadowmap_atlas.init(*p_physical_device, *p_device, data);
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
    VkImageView image_views[1] = { shadow_pass_texture.get_api_image_view() };
    create_frame_buffer(shadowpass.get_api_pass(), 1, image_views, shadowmap_width, shadowmap_height, &shadowpass_buffer);
}

void GraphicsImpl::create_swapchain_buffers() {
    size_t image_num = swapchain_images.size();
    swapchain_framebuffers.resize(image_num); 
    
    for (size_t i = 0; i < swapchain_images.size(); i++) {
        LOG("hi");
        VkImageView image_views[2] = { image_layers[0].get_api_image_view(), depth_memory.imageView };
        create_frame_buffer(render_pass.get_api_pass(), 2, image_views, swapchain_extent.width, swapchain_extent.height, &swapchain_framebuffers[i]);
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
    shadow_image_info.extent = VkExtent3D{ shadowmap_width, shadowmap_height, 1 };
    shadow_image_info.format = VK_FORMAT_D16_UNORM;
    shadow_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    shadow_image_info.queueFamilyIndexCount = 1;
    shadow_image_info.pQueueFamilyIndices = &graphics_family;

	VkImageViewUsageCreateInfo usageInfo{};
	usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
	usageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	//setup create struct for image views
	mem::ImageViewCreateInfo createInfo{};
	createInfo.pNext = &usageInfo;
	createInfo.aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

    mem::ImageData data{};
    data.image_info = shadow_image_info;
    data.image_view_info = createInfo;

    shadow_pass_texture.init(*p_physical_device, *p_device, data); 
}

void GraphicsImpl::create_oit_pipeline() {
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
    config.pass = render_pass.get_api_pass();
    config.subpass_index = 0;
    config.screen_extent = swapchain_extent;
    config.blend_colours = true;

    graphics_pipeline.init(*p_device, config);
}

void GraphicsImpl::create_oit_pass() {
    ColourConfig config{};
    config.format = swapchain_format;
    config.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    config.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    oit_pass.add_colour(0, config);
    oit_pass.create_subpass(VK_PIPELINE_BIND_POINT_GRAPHICS, true, false);
    oit_pass.build(*p_device, VK_PIPELINE_BIND_POINT_GRAPHICS);
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
    mem::createImage(*p_physical_device, *p_device, &depth_image_info, &depth_memory);

    //create image view
    VkImageViewUsageCreateInfo usageInfo{};
    usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
    usageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    //setup create struct for image views
    mem::ImageViewCreateInfo createInfo{};
    createInfo.pNext = &usageInfo;
    createInfo.view_type = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = VK_FORMAT_D16_UNORM;


    //this changes the colour output of the image, currently set to standard colours
    createInfo.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    //deciding on how many layers are in the image, and if we're using any mipmap levels.
    //layers are used for steroscopic 3d applications in which you would provide multiple images to each eye, creating a 3D effect.
    //mipmap levels are an optimization made so that lower quality textures are used when further away to save resources.
    createInfo.aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

    mem::createImageView(*p_device, createInfo, &depth_memory);
}

void GraphicsImpl::create_shadowpass() {
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

    DepthConfig config{};
    config.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    shadowpass.add_depth(0, config);
    shadowpass.add_dependency(dependencies);
    shadowpass.create_subpass(VK_PIPELINE_BIND_POINT_GRAPHICS, false, true);
    shadowpass.build(*p_device, VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void GraphicsImpl::create_deffered_textures() {
    VkImageCreateInfo image_info{};
}

void GraphicsImpl::create_geometry_buffer() {
    //create_frame_buffer(&g_buffer);
}

void GraphicsImpl::create_image_layers() {
    mem::ImageData data{};
    data.image_info.extent.width = swapchain_extent.width;
    data.image_info.extent.height = swapchain_extent.height;
    data.image_info.extent.depth = 1.0;

    data.image_info.format = swapchain_format;

    data.image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    data.image_info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    data.image_info.queueFamilyIndexCount = 1;
    data.image_info.pQueueFamilyIndices = &graphics_family;

    data.image_view_info.aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

    image_layers.resize(oit_layers);

    for (int i = 0; i < oit_layers; i++) {
        image_layers[i].init(*p_physical_device, *p_device, data);
    }
}

void GraphicsImpl::create_render_pass() {
    ColourConfig config{};
    config.format = swapchain_format;
    config.final_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    std::vector<VkSubpassDependency> dependencies(2);

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_NONE;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    render_pass.add_colour(0, config);
    render_pass.add_depth(1);
    render_pass.add_dependency(dependencies);
    render_pass.create_subpass(VK_PIPELINE_BIND_POINT_GRAPHICS, true, true);
    render_pass.build(*p_device, VK_PIPELINE_BIND_POINT_GRAPHICS);
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
        VK_DYNAMIC_STATE_DEPTH_BIAS
    };


    std::vector<VkDescriptorSetLayout> layouts = { light_layout };

    PipelineConfig config{};
    config.vert_shader_path = SHADER_PATH + "shadow.vert";
    config.dynamic_states = dynamic_states;
    config.descriptor_layouts = layouts;
    config.screen_extent = shadowmap_extent;
    config.pass = shadowpass.get_api_pass();
    config.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    config.subpass_index = 0;
    config.depth_bias_enable = VK_TRUE;

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
    config.pass = render_pass.get_api_pass();
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
 
    for (auto& buffer_data : shadowmap_buffers) {
        buffer_data.destroy(*p_device);
    }
    
    ubo_pool->destroyPool(*p_device);
    shadowmap_pool->destroyPool(*p_device);
    texture_pool->destroyPool(*p_device);

    uniform_buffer.destroy(*p_device);
    
    vkDestroyCommandPool(*p_device, command_pool, nullptr);
        
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
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*p_physical_device, surface, &capabilities) != VK_SUCCESS) {
        LOG("[ERROR] - create_swapchain : could not query the surface capabilities");
        throw std::runtime_error("");
    }

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*p_physical_device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(*p_physical_device, surface, &format_count, formats.data());

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
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
    mem::ImageViewCreateInfo image_info{};
    image_info.aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_info.format = swapchain_format;

    std::vector<VkImage> images;
    
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(*p_device, swapchain, &swapchain_image_count, nullptr);
    images.resize(swapchain_image_count);
    swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(*p_device, swapchain, &swapchain_image_count, images.data());

    mem::ImageData data;
    data.name = "swapchain";
    data.image_view_info = image_info;

    //transfer swapchain to present layout
    for (size_t i = 0; i < swapchain_image_count; i++) {
        swapchain_images[i].init(*p_physical_device, *p_device, images[i], data);
        swapchain_images[i].transfer(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, graphics_queue, command_pool);
    }
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

void GraphicsImpl::create_shadow_map(std::vector<GameObject*> game_objects, size_t command_index) {
    VkRenderPassBeginInfo shadowpass_info{};

    shadowpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    shadowpass_info.renderPass = shadowpass.get_api_pass();
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

    //NOTE: when imeplementing multiple shadowmaps, we can loop through MAX_SHADOW_CASTERS twice.
    //for (size_t y = 0; y < glm::sqrt(MAX_SHADOW_CASTERS); y++) {
    //    for (size_t x = 0; x < glm::sqrt(MAX_SHADOW_CASTERS); x++) {
            //there is at least one light that is casting a shadow here
    vkCmdBeginRenderPass(command_buffers[command_index], &shadowpass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport shadowpass_port = {};
    shadowpass_port.x = 0;
    shadowpass_port.y = 0;
    shadowpass_port.width = (float)shadowmap_width;
    shadowpass_port.height = (float)shadowmap_height;
    shadowpass_port.minDepth = 0.0;
    shadowpass_port.maxDepth = 1.0;
    vkCmdSetViewport(command_buffers[command_index], 0, 1, &shadowpass_port);

    VkRect2D shadowpass_scissor{};
    shadowpass_scissor.offset = { 0, 0 };
    shadowpass_scissor.extent.width = shadowmap_width;
    shadowpass_scissor.extent.height = shadowmap_height;
    vkCmdSetScissor(command_buffers[command_index], 0, 1, &shadowpass_scissor);

    vkCmdSetDepthBias(command_buffers[command_index], depth_bias_constant, 0.0f, depth_bias_slope);

    vkCmdBindPipeline(command_buffers[command_index], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowmap_pipeline.get_api_pipeline());

    //time for the draw calls
    const VkDeviceSize offsets[] = { 0, offsetof(Vertex, normal), offsetof(Vertex, tex_coord) };
    vkCmdBindVertexBuffers(command_buffers[command_index], 0, 1, &vertex_buffer.buffer, offsets);
    vkCmdBindIndexBuffer(command_buffers[command_index], index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    uint32_t total_indexes = 0;
    uint32_t total_vertices = 0;

    uint32_t* number;

    for (size_t j = 0; j < game_objects.size(); j++) {
        if (!game_objects[j]->update) {
            for (size_t k = 0; k < game_objects[j]->object_model.model_meshes.size(); k++) {
                std::shared_ptr<Mesh> mesh_data = game_objects[j]->object_model.model_meshes[k];
                uint32_t index_count = static_cast<uint32_t>(mesh_data->indices.size());
                uint32_t vertex_count = static_cast<uint32_t>(mesh_data->vertices.size());

                VkDescriptorSet descriptors[1] = { light_ubo[j][command_index] };
                vkCmdBindDescriptorSets(command_buffers[command_index],
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    shadowmap_pipeline.get_api_layout(),
                    0,
                    1,
                    descriptors,
                    0,
                    nullptr);

                vkCmdDrawIndexed(command_buffers[command_index], 
                    index_count, 
                    1, 
                    total_indexes, 
                    total_vertices, 
                    static_cast<uint32_t>(0));

                total_indexes += index_count;
                total_vertices += vertex_count;
            }
        }
    }
    vkCmdEndRenderPass(command_buffers[command_index]);
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

        create_shadow_map(game_objects, i);
         
		VkRenderPassBeginInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderInfo.renderPass = render_pass.get_api_pass();
		renderInfo.framebuffer = swapchain_framebuffers[i];

        VkRect2D render_area{};
		render_area.offset = VkOffset2D{ 0, 0 };
		render_area.extent = swapchain_extent;
		renderInfo.renderArea = render_area;

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

        std::vector<TransparentMesh> transparent_meshes;

        for (size_t j = 0; j < game_objects.size(); j++) {
            if (!game_objects[j]->update) {
                for (size_t k = 0; k < game_objects[j]->object_model.model_meshes.size(); k++) {
                    std::shared_ptr<Mesh> mesh_data = game_objects[j]->object_model.model_meshes[k];
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

        vkCmdEndRenderPass(command_buffers[i]);
        //switch image back to depth stencil layout for the next render pass

        resolve_image_layers(i);
   
        //end commands to go to execute stage
        if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("could not end command buffer");
        }
    }
}

void GraphicsImpl::resolve_image_layers(size_t i) {
    //can use VkSubpassDependency to change layout of image_layers[0] automatically.
    swapchain_images[i].transfer(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, graphics_queue, command_pool, command_buffers[i]);

    VkImageCopy copy_info{};

    VkImageSubresourceLayers src_subresource{};
    src_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    src_subresource.baseArrayLayer = 0;
    src_subresource.layerCount = 1;
    src_subresource.mipLevel = 0;

    VkOffset3D src_offset{};
    src_offset.x = 0;
    src_offset.y = 0;
    src_offset.z = 0;

    VkImageSubresourceLayers dst_subresource{};
    dst_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    dst_subresource.baseArrayLayer = 0;
    dst_subresource.layerCount = 1;
    dst_subresource.mipLevel = 0;

    VkOffset3D dst_offset{};
    dst_offset.x = 0;
    dst_offset.y = 0;
    dst_offset.z = 0;

    VkExtent3D size{};
    size.depth = 1;
    size.height = swapchain_extent.height;
    size.width = swapchain_extent.width;

    copy_info.extent = size;
    copy_info.dstOffset = dst_offset;
    copy_info.srcOffset = src_offset;
    copy_info.srcSubresource = src_subresource;
    copy_info.dstSubresource = dst_subresource;

    vkCmdCopyImage(command_buffers[i],
        image_layers[0].get_api_image(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapchain_images[i].get_api_image(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy_info);


    swapchain_images[i].transfer(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, graphics_queue, command_pool, command_buffers[i]);
}

void GraphicsImpl::create_uniform_buffer() {
    mem::BufferCreateInfo buffer_info{};
    buffer_info.size = (VkDeviceSize)BUFFER_SIZE;
    buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uniform_buffer.init(*p_physical_device, *p_device, &buffer_info);
}

void GraphicsImpl::create_vertex_buffer() {
    mem::BufferCreateInfo buffer_info{};
    buffer_info.size = (VkDeviceSize)BUFFER_SIZE;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vertex_buffer.init(p_physical_device.get(), &*p_device, &command_pool, &buffer_info);
}

void GraphicsImpl::create_index_buffer() {
    mem::BufferCreateInfo buffer_info{};
    buffer_info.size = (VkDeviceSize)BUFFER_SIZE;
    buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    index_buffer.init(&*p_physical_device, &*p_device, &command_pool, &buffer_info);
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
    VkCommandBuffer transferBuffer = begin_command_buffer(*p_device, command_pool);

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
    end_command_buffer(*p_device, graphics_queue, command_pool, transferBuffer);
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

    mem::destroyImage(*p_device, depth_memory);
    render_pass.destroy();
    vkDestroySwapchainKHR(*p_device, swapchain, nullptr);
}

/// <summary>
/// When called, this function create all necesarry objects for the swapchain
/// </summary>
void GraphicsImpl::recreate_swapchain() {
    create_swapchain();
    create_depth_resources();
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

void GraphicsImpl::write_to_shadowmap_set() {
    VkDescriptorImageInfo imageInfo;
    imageInfo.sampler = shadowmap_sampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = shadow_pass_texture.get_api_image_view();

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
    //shadowmap_atlas.transfer(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, graphics_queue, command_pool);
}

void GraphicsImpl::write_to_texture_set(VkDescriptorSet texture_set, VkImageView image_view) {
    VkDescriptorImageInfo imageInfo;
    imageInfo.sampler = texture_sampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image_view;

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
    texture_images.resize(texture_images.size() + 1);
    texture_images[texture_images.size() - 1].resize(1);

    mem::ImageCreateInfo imageInfo{};
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    VkExtent3D extent{};
    extent.width = 1;
    extent.height = 1;
    extent.depth = 1; //this depth might be key to blending the textures...
    imageInfo.extent = extent;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.queueFamilyIndexCount = 1;
    imageInfo.pQueueFamilyIndices = &graphics_family;
    imageInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    mem::ImageViewCreateInfo viewInfo{};
    viewInfo.aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

    mem::ImageData data{};
    data.name = "empty";
    data.image_info = imageInfo;
    data.image_view_info = viewInfo;
 
    texture_images[texture_images.size() - 1][0].init(*p_physical_device, *p_device, data);

    //transfer the image again to a more optimal layout for texture sampling?
    texture_images[texture_images.size() - 1][0].transfer(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, graphics_queue, command_pool);
    //create image view for image

    //save texture image to mesh

    //write to texture set
    write_to_texture_set(texture_sets[object][texture_set], texture_images[texture_images.size() - 1][0].get_api_image_view());    
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
    mem::createBuffer(*p_physical_device, *p_device, &textureBufferInfo, &newBuffer);
    
    mem::allocateMemory(dataSize, &newBuffer);
    mem::mapMemory(*p_device, dataSize, &newBuffer, pixels);

    //use size of loaded image to create VkImage
    texture_images.resize(texture_images.size() + 1);
    texture_images[texture_images.size() - 1].resize(1);

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

    mem::ImageViewCreateInfo viewInfo{};
    viewInfo.aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

    mem::ImageData data{};
    data.name = "texture";
    data.image_info = imageInfo;
    data.image_view_info = viewInfo;

    texture_images[texture_images.size() - 1][0].init(*p_physical_device, *p_device, data);
    

    //mem::maAllocateMemory(dataSize, &newTextureImage);
    //transfer the image to appropriate layout for copying
   
    texture_images[texture_images.size() - 1][0].transfer(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, graphics_queue, command_pool);
    VkOffset3D image_offset = {0, 0, 0};
    texture_images[texture_images.size() - 1][0].copy_from_buffer(newBuffer.buffer, image_offset, std::nullopt, graphics_queue, command_pool);
    texture_images[texture_images.size() - 1][0].transfer(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, graphics_queue, command_pool);
    //transfer the image again to a more optimal layout for texture sampling?

    //create image view for image

    mem::destroyBuffer(*p_device, newBuffer);

    //save texture image to mesh
    mem::ImageSize image_dimensions{};
    image_dimensions.width = static_cast<uint32_t>(imageWidth);
    image_dimensions.height = static_cast<uint32_t>(imageHeight);

    //write to texture set
    write_to_texture_set(texture_sets[object][texture_set], texture_images[texture_images.size() - 1][0].get_api_image_view());

    //free image
    stbi_image_free(pixels);
}
