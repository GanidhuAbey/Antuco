//this will handle setting up the graphics pipeline, recording command buffers, and generally anything requiring rendering within
//the vulkan framework will end up here
#include "api_graphics.hpp"

#include "memory_allocator.hpp"
#include "data_structures.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>

#include <stdexcept>

const uint32_t MAX_FRAMES_IN_FLIGHT = 3;

using namespace tuco;

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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &texture_sampler) != VK_SUCCESS) {
        printf("[ERROR] - createTextureSampler() : failed to create sampler object");
    }
}

void GraphicsImpl::create_frame_buffers() {
    //get the number of images we need to create framebuffers for
    size_t imageNum = swapchain_images.size();
    //resize framebuffer vector to fit as many frame buffers as images
    swapchain_framebuffers.resize(imageNum);

    //iterate through all the frame buffers
    for (int i = 0; i < imageNum; i++) {
        //create a frame buffer
        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = render_pass;
        //we only want one image per frame buffer
        createInfo.attachmentCount = 2;
        //they put the image view in a separate array for some reason
        VkImageView imageViews[2] = { swapchain_image_views[i], depth_memory.imageView };
        createInfo.pAttachments = imageViews;
        createInfo.width = swapchain_extent.width;
        createInfo.height = swapchain_extent.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapchain_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("could not create a frame buffer");
        }
    }
}

void GraphicsImpl::create_semaphores() {
    //create required sephamores
    VkSemaphoreCreateInfo semaphoreBegin{};
    semaphoreBegin.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreBegin, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreBegin, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS) {
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
        if (vkCreateFence(device, &createInfo, nullptr, &in_flight_fences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fences");
        };
    }
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
    mem::createImage(physical_device, device, &depth_image_info, &depth_memory);

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
    //TODO: come back here when you know what those mean
    //layers are used for steroscopic 3d applications in which you would provide multiple images to each eye, creating a 3D effect.
    //mipmap levels are an optimization made so that lower quality textures are used when further away to save resources.
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    mem::createImageView(device, createInfo, &depth_memory);
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
        //TODO: come back here when you know what those mean
        //layers are used for steroscopic 3d applications in which you would provide multiple images to each eye, creating a 3D effect.
        //mipmap levels are an optimization made so that lower quality textures are used when further away to save resources.
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapchain_image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("one of the image views could not be created");
        }
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

    //TODO: dont know much about stencilling, seems to be something about colouring in the image from a different layer?
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    //TODO: what does the undefined here mean? that it can be anything?
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

    VkSubpassDescription depthSubpass{};
    depthSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    depthSubpass.pDepthStencilAttachment = &depthAttachmentRef;

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
    VkSubpassDescription subpasses[1] = { colorSubpass };
    createInfo.pSubpasses = subpasses;
    //createInfo.dependencyCount = 1;
    //createInfo.pDependencies = &dependency;


    if (vkCreateRenderPass(device, &createInfo, nullptr, &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("could not create render pass");
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

    if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &ubo_layout) != VK_SUCCESS) {
        throw std::runtime_error("could not create ubo layout");
    }
}

size_t GraphicsImpl::create_ubo_pool() {
    //the previous system created both pools and descriptor sets as they were needed for new objects
    //the choices are to create new pools/descriptor sets for every object, or to reuse old descroptor sets/pools when needed...

    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = swapchain_images.size(); 
    
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = swapchain_images.size();
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    size_t current_size = ubo_pools.size();
    ubo_pools.resize(current_size + 1);
    vkCreateDescriptorPool(device, &pool_info, nullptr, &ubo_pools[current_size]);
    
    //this doesn't make the removal system easy...
    return current_size;
}

void GraphicsImpl::create_ubo_set() {
    std::vector<VkDescriptorSetLayout> ubo_layouts(swapchain_images.size(), ubo_layout);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = ubo_pools[ubo_pools.size() - 1];
    allocateInfo.descriptorSetCount = swapchain_images.size();
    allocateInfo.pSetLayouts = ubo_layouts.data();

    //size_t currentSize = descriptorSets.size();
    //descriptorSets.resize(currentSize + 1);
    //descriptorSets[currentSize].resize(swapChainImages.size());

    size_t current_size = ubo_sets.size();
    ubo_sets.resize(current_size + 1);
    ubo_sets[current_size].resize(swapchain_images.size());
    if (vkAllocateDescriptorSets(device, &allocateInfo, ubo_sets[current_size].data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    } 
}

void GraphicsImpl::create_texture_pool(size_t mesh_count) {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = swapchain_images.size() * mesh_count;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.pNext = nullptr;
    pool_info.maxSets = swapchain_images.size() * mesh_count;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    size_t current_size = texture_pools.size();
    texture_pools.resize(current_size + 1);
    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &texture_pools[current_size]) != VK_SUCCESS) {
        printf("[ERROR] - create_texture_pool : could not create texture pool");
        throw std::runtime_error("");
    }
}

void GraphicsImpl::create_texture_set(size_t mesh_count) { 
    std::vector<VkDescriptorSetLayout> texture_layouts(swapchain_images.size(), texture_layout);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = texture_pools[texture_pools.size() - 1];
    allocateInfo.descriptorSetCount = swapchain_images.size();
    allocateInfo.pSetLayouts = texture_layouts.data();

    //size_t currentSize = descriptorSets.size();
    //descriptorSets.resize(currentSize + 1);
    //descriptorSets[currentSize].resize(swapChainImages.size());

    size_t current_size = texture_sets.size();
    texture_sets.resize(current_size + 1);
    texture_sets[current_size].resize(mesh_count);

    for (size_t i = 0; i < mesh_count; i++) {
        texture_sets[current_size][i].resize(swapchain_images.size());
        if (vkAllocateDescriptorSets(device, &allocateInfo, texture_sets[current_size][i].data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate texture sets!");
        }
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

    if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &texture_layout) != VK_SUCCESS) {
        throw std::runtime_error("could not create texture layout");
    }
}




std::vector<char> GraphicsImpl::read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        printf("could not open file \n");
        throw std::runtime_error("could not open file");
    }

    size_t fileSize = (size_t)file.tellg();

    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule GraphicsImpl::create_shader_module(std::vector<char> shaderCode) {
    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = static_cast<uint32_t>(shaderCode.size());

    const uint32_t* shaderFormatted = reinterpret_cast<const uint32_t*>(shaderCode.data());

    createInfo.pCode = shaderFormatted;

    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
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

void GraphicsImpl::create_graphics_pipeline() {
    //load in the appropriate shader code for a triangle
    auto vertShaderCode = read_file("shaders/vert.spv");
    auto fragShaderCode = read_file("shaders/frag.spv");

    //convert the shader code into a vulkan object
    VkShaderModule vertShader = create_shader_module(vertShaderCode);
    VkShaderModule fragShader = create_shader_module(fragShaderCode);

    //create shader stage of the graphics pipeline
    VkPipelineShaderStageCreateInfo createVertShaderInfo = fill_shader_stage_struct(VK_SHADER_STAGE_VERTEX_BIT, vertShader);
    VkPipelineShaderStageCreateInfo createFragShaderInfo = fill_shader_stage_struct(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader);

    const VkPipelineShaderStageCreateInfo shaderStages[] = { createVertShaderInfo, createFragShaderInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;

    VkVertexInputBindingDescription bindingDescrip{};
    bindingDescrip.binding = 0;
    bindingDescrip.stride = sizeof(Vertex);
    bindingDescrip.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescrip;

    vertexInputInfo.vertexAttributeDescriptionCount = 3;

    VkVertexInputAttributeDescription posAttribute{};
    posAttribute.location = 0;
    posAttribute.binding = 0;
    posAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    posAttribute.offset = 0;

    /*
    VkVertexInputAttributeDescription colorAttribute{};
    colorAttribute.location = 1;
    colorAttribute.binding = 0;
    colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(data::Vertex, color);
    */

    VkVertexInputAttributeDescription normalAttribute{};
    normalAttribute.location = 1;
    normalAttribute.binding = 0;
    normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset = offsetof(Vertex, normal);

    VkVertexInputAttributeDescription texAttribute{};
    texAttribute.location = 2;
    texAttribute.binding = 0;
    texAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    texAttribute.offset = offsetof(Vertex, tex_coord);

    VkVertexInputAttributeDescription attributeDescriptions[] = { posAttribute, normalAttribute, texAttribute };

    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)swapchain_extent.width;
    viewport.height = (float)swapchain_extent.height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    VkRect2D scissor{   };
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain_extent;

    VkPipelineViewportStateCreateInfo viewportInfo{};

    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.depthClampEnable = VK_FALSE;
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    //TODO: try to enable the wideLines gpu feature
    rasterizationInfo.lineWidth = 1.0f;
    rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationInfo.depthBiasEnable = VK_FALSE;
    rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
    rasterizationInfo.depthBiasClamp = 0.0f; // Optional
    rasterizationInfo.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.pNext = nullptr;
    depthStencilInfo.flags = 0;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.stencilTestEnable = VK_FALSE;

    float blendValues[4] = { 0.0, 0.0, 0.0, 0.5 };
    colorBlendInfo.blendConstants[0] = blendValues[0];
    colorBlendInfo.blendConstants[1] = blendValues[1];
    colorBlendInfo.blendConstants[2] = blendValues[2];
    colorBlendInfo.blendConstants[3] = blendValues[3];

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = 2;
    dynamicInfo.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    const uint32_t layoutCount = 2;
    pipelineLayoutInfo.setLayoutCount = layoutCount;
    VkDescriptorSetLayout layouts[layoutCount] = { ubo_layout, texture_layout };
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(LightObject);

    VkPushConstantRange pushRanges[] = { pushRange };

    pipelineLayoutInfo.pPushConstantRanges = pushRanges;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("could not create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo createGraphicsPipelineInfo{};
    createGraphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createGraphicsPipelineInfo.stageCount = 2;
    createGraphicsPipelineInfo.pStages = shaderStages;
    createGraphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
    createGraphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    createGraphicsPipelineInfo.pViewportState = &viewportInfo;
    createGraphicsPipelineInfo.pRasterizationState = &rasterizationInfo;
    createGraphicsPipelineInfo.pMultisampleState = &multisampling;
    createGraphicsPipelineInfo.pColorBlendState = &colorBlendInfo;
    createGraphicsPipelineInfo.pDepthStencilState = &depthStencilInfo;
    createGraphicsPipelineInfo.pDynamicState = &dynamicInfo;
    createGraphicsPipelineInfo.layout = pipeline_layout;
    createGraphicsPipelineInfo.renderPass = render_pass;
    createGraphicsPipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createGraphicsPipelineInfo, nullptr, &graphics_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("could not create graphics pipeline");
    }

    //destroy the used shader object
    vkDestroyShaderModule(device, vertShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);

}

void GraphicsImpl::write_to_ubo() {
    mem::allocateMemory((VkDeviceSize)sizeof(UniformBufferObject), &uniform_buffer);

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = uniform_buffer.buffer;
    buffer_info.offset = uniform_buffer.offset;
    buffer_info.range = (VkDeviceSize)sizeof(UniformBufferObject);

    for (size_t i = 0; i < swapchain_images.size(); i++) {
        VkWriteDescriptorSet writeInfo{};
        writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeInfo.dstBinding = 0;
        writeInfo.dstSet = ubo_sets[ubo_pools.size() - 1][i];
        writeInfo.descriptorCount = 1;
        writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeInfo.pBufferInfo = &buffer_info;
        writeInfo.dstArrayElement = 0;

        vkUpdateDescriptorSets(device, 1, &writeInfo, 0, nullptr);
    }
}

void GraphicsImpl::destroy_draw() {
    vkDeviceWaitIdle(device);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(device, in_flight_fences[i], nullptr);
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
    }

    for (size_t i = 0; i < swapchain_images.size(); i++) {
        vkDestroyImageView(device, swapchain_image_views[i], nullptr);
    }

    for (size_t i = 0; i < texture_images.size(); i++) {
        for (size_t j = 0; j < texture_images[i].size(); j++) {
            printf("hey \n");
            mem::destroyImage(device, *texture_images[i][j]);
        }
    }

    for (size_t i = 0; i < ubo_pools.size(); i++) {
        vkDestroyDescriptorPool(device, ubo_pools[i], nullptr);
    }

    for (size_t i = 0; i < texture_pools.size(); i++) {
        vkDestroyDescriptorPool(device, texture_pools[i], nullptr);
    }

    mem::destroyBuffer(device, vertex_buffer);
    mem::destroyBuffer(device, uniform_buffer);
    mem::destroyBuffer(device, index_buffer);
    
    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyPipeline(device, graphics_pipeline, nullptr);

    vkDestroyRenderPass(device, render_pass, nullptr);

    vkDestroyDescriptorSetLayout(device, ubo_layout, nullptr);
    vkDestroyDescriptorSetLayout(device, texture_layout, nullptr);

    vkDestroySampler(device, texture_sampler, nullptr);

    for (int i = 0; i < swapchain_framebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapchain_framebuffers[i], nullptr);
    }

    mem::destroyImage(device, depth_memory);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
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
        printf("[ERROR] - create_swapchain : could not query the surface capabilities");
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

    if (vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain) != VK_SUCCESS) {
        printf("[ERROR] - create_swapchain : could not create swapchain \n");
        throw std::runtime_error("");
    }

    //grab swapchain images
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
    swapchain_images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());

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

    if (vkAllocateCommandBuffers(device, &bufferAllocate, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate memory for command buffers");
    }

    //push all my command buffers into an exectute stage.
    //TODO: multithread this process

    for (size_t i = 0; i < command_buffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(command_buffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("one of the command buffers failed to begin");
        }
        //begin a render pass so that we can draw to the appropriate framebuffer
        VkRenderPassBeginInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderInfo.renderPass = render_pass;
        renderInfo.framebuffer = swapchain_framebuffers[i];
        VkRect2D renderArea{};
        renderArea.offset = VkOffset2D{ 0, 0 };
        renderArea.extent = swapchain_extent;
        renderInfo.renderArea = renderArea;

        const size_t size_of_array = 2;
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
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

        VkViewport newViewport = {};
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
        const VkDeviceSize offsets[] = { 0, offsetof(Vertex, normal), offsetof(Vertex, tex_coord) };
        vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffer.buffer, offsets);
        vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        //universal to every object so i can push the light constants before the for loop
        vkCmdPushConstants(command_buffers[i], pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(light), &light);
        //draw first object (cube)
        uint32_t total_indexes = 0;
        uint32_t total_vertices = 0;

        for (size_t j = 0; j < game_objects.size(); j++) {
            for (size_t k = 0; k < game_objects[j]->object_model.model_meshes.size(); k++) {
                Mesh* mesh_data = game_objects[j]->object_model.model_meshes[k];
                uint32_t index_count = static_cast<uint32_t>(mesh_data->indices.size());
                uint32_t vertex_count = static_cast<uint32_t>(mesh_data->vertices.size());

                //we're kinda phasing object colours out with the introduction of textures, so i'm probably not gonna need to push this
                //vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(light), sizeof(pfcs[i]), &pfcs[i]);

                VkDescriptorSet descriptors[2] = {ubo_sets[j][i], texture_sets[j][k][i]};
                vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 2, descriptors, 0, nullptr);
                vkCmdDrawIndexed(command_buffers[i], index_count, 1, total_indexes, total_vertices, static_cast<uint32_t>(0));

                total_indexes += index_count;
                total_vertices += vertex_count;
            }
        }

        //vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(6), 1, 36, 0, 0);
        //vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(command_buffers[i]);

        //end commands to go to execute stage
        if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("could not end command buffer");
        }
    }
}


void GraphicsImpl::create_uniform_buffer() {
    mem::BufferCreateInfo buffer_info{};
    buffer_info.size = (VkDeviceSize)5e8;
    buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    mem::createBuffer(physical_device, device, &buffer_info, &uniform_buffer);
}

void GraphicsImpl::create_vertex_buffer() {
    mem::BufferCreateInfo buffer_info{};
    buffer_info.size = (VkDeviceSize)5e8;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    mem::createBuffer(physical_device, device, &buffer_info, &vertex_buffer);
}

void GraphicsImpl::create_index_buffer() {
    mem::BufferCreateInfo buffer_info{};
    buffer_info.size = (VkDeviceSize)5e8;
    buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &graphics_family;
    buffer_info.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    mem::createBuffer(physical_device, device, &buffer_info, &index_buffer);
}

void GraphicsImpl::update_index_buffer(std::vector<uint32_t> indices_data) {
    //create temporary cpu readable buffer
    mem::BufferCreateInfo temp_info{};
    temp_info.size = sizeof(indices_data[0]) * indices_data.size();
    temp_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    temp_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    temp_info.queueFamilyIndexCount = 1;
    temp_info.pQueueFamilyIndices = &graphics_family;
    temp_info.memoryProperties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    mem::Memory temp_buffer;
    mem::createBuffer(physical_device, device, &temp_info, &temp_buffer);

    //map data to temp buffer
    mem::allocateMemory(sizeof(indices_data[0]) * indices_data.size(), &temp_buffer);
    mem::mapMemory(device, sizeof(indices_data[0]) * indices_data.size(), &temp_buffer, indices_data.data());

    //use command buffers to transfer data to device local buffer
    mem::allocateMemory(sizeof(indices_data[0]) * indices_data.size(), &index_buffer);
    copy_buffer(temp_buffer, index_buffer, index_buffer.offset, sizeof(indices_data[0]) * indices_data.size());
    index_buffer.allocate = false;

    //destroy old temp cpu
    mem::destroyBuffer(device, temp_buffer);

}

void GraphicsImpl::update_vertex_buffer(std::vector<Vertex> vertex_data) {
    //create temporary cpu readable buffer
    mem::BufferCreateInfo temp_info{};
    temp_info.size = sizeof(vertex_data[0]) * vertex_data.size();
    temp_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    temp_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    temp_info.queueFamilyIndexCount = 1;
    temp_info.pQueueFamilyIndices = &graphics_family;
    temp_info.memoryProperties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    mem::Memory temp_buffer;
    mem::createBuffer(physical_device, device, &temp_info, &temp_buffer);

    //map data to temp buffer
    for (size_t i = 0; i < vertex_data.size(); i++) {
        printf("vertex_data: <%f, %f, %f> \n", vertex_data[i].position.x, vertex_data[i].position.y, vertex_data[i].position.z);
    }
    mem::allocateMemory(sizeof(vertex_data[0]) * vertex_data.size(), &temp_buffer);
   
    /* THIS CODE FOR SURE WORKS*/
    //void* p_data;
    //vkMapMemory(device, temp_buffer.memoryHandle, 0, sizeof(vertex_data[0]) * vertex_data.size(), 0, &p_data);
    //memcpy(p_data, vertex_data.data(), sizeof(vertex_data[0]) * vertex_data.size());V

    mem::mapMemory(device, sizeof(vertex_data[0]) * vertex_data.size(), &temp_buffer, vertex_data.data());

    //use command buffers to transfer data to device local buffer 
    mem::allocateMemory(sizeof(vertex_data[0]) * vertex_data.size(), &vertex_buffer);
    copy_buffer(temp_buffer, vertex_buffer, vertex_buffer.offset, sizeof(vertex_data[0]) * vertex_data.size());
    vertex_buffer.allocate = false;

    //destroy old temp cpu
    mem::destroyBuffer(device, temp_buffer);

}

void GraphicsImpl::copy_buffer(mem::Memory src_buffer, mem::Memory dst_buffer, VkDeviceSize dst_offset, VkDeviceSize data_size) {
    VkCommandBuffer transferBuffer = begin_command_buffer();

    //transfer between buffers
    VkBufferCopy copyData{};
    copyData.srcOffset = 0;
    //TODO: need to allocate memory and choose a proper offset for this
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

    if (vkAllocateCommandBuffers(device, &bufferAllocate, &transferBuffer) != VK_SUCCESS) {
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

    vkFreeCommandBuffers(device, command_pool, 1, &commandBuffer);
}

void GraphicsImpl::update_uniform_buffer(VkDeviceSize memory_offset, UniformBufferObject ubo) {
    //free current memory
    mem::FreeMemoryInfo free_info{};
    free_info.deleteOffset = memory_offset;
    free_info.deleteSize = sizeof(ubo);
    mem::freeMemory(free_info, &uniform_buffer);

    //allocate memory and map new data
    mem::allocateMemory(sizeof(ubo), &uniform_buffer, &memory_offset);
    mem::mapMemory(device, sizeof(ubo), &uniform_buffer, &ubo);
}



void GraphicsImpl::draw_frame() {
    //make sure that the current frame thats being drawn in parallel is available
    vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
    //allocate memory to store next image
    uint32_t nextImage;

    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &nextImage);

    /* Window Resizing Functionality: will implement after finishing rendering TODO
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        cleanupSwapChain(false);
        //VkSwapchainKHR oldSwapChain = swapChain;
        recreateSwapChain();

        return;
    }
    */
    if (result != VK_SUCCESS) {
        throw std::runtime_error("could not aquire image from swapchain");
    }

    if (images_in_flight[nextImage] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &images_in_flight[nextImage], VK_TRUE, UINT64_MAX);
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

    vkResetFences(device, 1, &in_flight_fences[current_frame]);

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

    /*
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        cleanupSwapChain(false);
        recreateSwapChain();
    }
    */
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void GraphicsImpl::transfer_image_layout(VkImageLayout initial_layout, VkImageLayout output_layout, mem::Memory* image) {
    //begin command buffer
    VkCommandBuffer commandBuffer = begin_command_buffer();


    //transfer image layout
    VkImageMemoryBarrier imageTransfer{};
    imageTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageTransfer.pNext = nullptr;
    imageTransfer.oldLayout = initial_layout;
    imageTransfer.newLayout = output_layout;
    imageTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageTransfer.image = image->image;
    imageTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageTransfer.subresourceRange.baseMipLevel = 0;
    imageTransfer.subresourceRange.levelCount = 1;
    imageTransfer.subresourceRange.baseArrayLayer = 0;
    imageTransfer.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    if (output_layout == VK_IMAGE_LAYOUT_UNDEFINED && initial_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        imageTransfer.srcAccessMask = 0; // this basically means none or doesnt matter
        imageTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageTransfer
    );

    //end command buffer

    end_command_buffer(commandBuffer);
}

void GraphicsImpl::copy_image(mem::Memory buffer, mem::Memory image, VkDeviceSize dst_offset, uint32_t image_width, uint32_t image_height) {
        //create command buffer
    VkCommandBuffer transferBuffer = begin_command_buffer();

    VkImageSubresourceLayers imageSub{};
    imageSub.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSub.mipLevel = 0;
    imageSub.baseArrayLayer = 0;
    imageSub.layerCount = 1;

    VkOffset3D imageOffset = {
        0,
        0,
        0,
    };

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
    bufferCopy.imageOffset = imageOffset;
    bufferCopy.imageExtent = imageExtent;

    vkCmdCopyBufferToImage(transferBuffer,
        buffer.buffer,
        image.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bufferCopy
    );

    //destroy transfer buffer, shouldnt need it after copying the data.
    end_command_buffer(transferBuffer);
}

void GraphicsImpl::write_to_texture_set(std::vector<VkDescriptorSet> texture_sets, mem::Memory* image) {
    VkDescriptorImageInfo imageInfo;
    imageInfo.sampler = texture_sampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image->imageView;

    for (const auto& texture_set : texture_sets) {
        VkWriteDescriptorSet writeInfo{};
        writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeInfo.dstBinding = 0;
        writeInfo.dstSet = texture_set;
        writeInfo.descriptorCount = 1;
        writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeInfo.pImageInfo = &imageInfo;
        writeInfo.dstArrayElement = 0;

        vkUpdateDescriptorSets(device, 1, &writeInfo, 0, nullptr);
    }
}


void GraphicsImpl::create_texture_image(aiString texturePath, size_t object, size_t texture_set) {
    int imageWidth, imageHeight, imageChannels;
    stbi_uc *pixels = stbi_load(texturePath.data, &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);

    VkDeviceSize dataSize = 4 * (imageWidth * imageHeight);
    mem::BufferCreateInfo textureBufferInfo{};
    textureBufferInfo.size = dataSize;
    textureBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    textureBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    textureBufferInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    textureBufferInfo.queueFamilyIndexCount = 1;
    textureBufferInfo.pQueueFamilyIndices = &graphics_family;

    mem::Memory newBuffer;
    mem::createBuffer(physical_device, device, &textureBufferInfo, &newBuffer);
    
    mem::allocateMemory(dataSize, &newBuffer);
    mem::mapMemory(device, dataSize, &newBuffer, pixels);

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

    mem::createImage(physical_device, device, &imageInfo, new_texture_image);

    //mem::maAllocateMemory(dataSize, &newTextureImage);
    //transfer the image to appropriate layout for copying
    transfer_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, new_texture_image);
    copy_image(newBuffer, *new_texture_image, 0.0, imageWidth, imageHeight);
    //transfer the image again to a more optimal layout for texture sampling?
    transfer_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, new_texture_image);

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

    mem::createImageView(device, viewInfo, new_texture_image);

    mem::destroyBuffer(device, newBuffer);

    //save texture image to mesh
    mem::ImageSize image_dimensions{};
    image_dimensions.width = static_cast<uint32_t>(imageWidth);
    image_dimensions.height = static_cast<uint32_t>(imageHeight);

    texture_images.resize(texture_pools.size());
    texture_images[texture_pools.size() - 1].push_back(new_texture_image);

    //write to texture set
    write_to_texture_set(texture_sets[object][texture_set], new_texture_image);

    //free image
    stbi_image_free(pixels);
}