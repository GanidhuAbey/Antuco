#include "memory_allocator.hpp"
#include "vulkan/vulkan_core.h"
#include "queue.hpp"
#include "logger/interface.hpp"
#include "api_config.hpp"

#include <bitset>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace mem;


SearchBuffer::SearchBuffer() {}
SearchBuffer::~SearchBuffer() {
    //destroy(api_device);
}

void SearchBuffer::init(VkPhysicalDevice physical_device, VkDevice& device, BufferCreateInfo* p_buffer_info) { 
    api_device = device;

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext = p_buffer_info->pNext;
    createInfo.flags = p_buffer_info->flags;
    createInfo.usage = p_buffer_info->usage;
    createInfo.size = p_buffer_info->size;
    createInfo.sharingMode = p_buffer_info->sharingMode;
    createInfo.queueFamilyIndexCount = p_buffer_info->queueFamilyIndexCount;
    createInfo.pQueueFamilyIndices = p_buffer_info->pQueueFamilyIndices;

    if (vkCreateBuffer(device, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create buffer");
    };

    //allocate desired memory to buffer
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    //allocate memory for buffer
    VkMemoryAllocateInfo memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.allocationSize = memRequirements.size;
    //too lazy to even check if this exists will do later TODO
    memoryInfo.memoryTypeIndex = findMemoryType(physical_device, memRequirements.memoryTypeBits, p_buffer_info->memoryProperties);

    VkResult allocResult = vkAllocateMemory(device, &memoryInfo, nullptr, &buffer_memory);

    if (allocResult != VK_SUCCESS) {
        throw std::runtime_error("could not allocate memory for memory pool");
    }

    if (vkBindBufferMemory(device, buffer, buffer_memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("could not bind allocated memory to buffer");
    }

    memory_offset = 0;
}

void SearchBuffer::destroy() {
    vkDeviceWaitIdle(api_device);

    vkFreeMemory(api_device, buffer_memory, nullptr);

    vkDestroyBuffer(api_device, buffer, nullptr);
}

VkDeviceSize SearchBuffer::allocate(VkDeviceSize allocation_size) {
    VkDeviceSize offset = memory_offset;
    memory_locations.push_back(offset);

    memory_offset += allocation_size;

    return offset;
}

void SearchBuffer::write(VkDevice device, VkDeviceSize offset, VkDeviceSize data_size, void* p_data) {
    void* pData;
    if (vkMapMemory(device, buffer_memory, offset, data_size, 0, &pData) != VK_SUCCESS) {
        throw std::runtime_error("could not map data to memory");
    }
    memcpy(pData, p_data, data_size);
    vkUnmapMemory(device, buffer_memory);
}

//this almost never needed when dealing with uniform buffer, so i'm not gonna write this function for now
void SearchBuffer::free(VkDeviceSize offset) {
}


Image::Image() {}

Image::~Image() {
    //destroy();
}

void Image::destroy() {
    LOG(data.name.c_str());
    destroy_image_view();
    vkFreeMemory(device, memory, nullptr);
    vkDestroyImage(device, image, nullptr);
}

void Image::destroy_image_view() {
    vkDestroyImageView(device, image_view, nullptr);
}

VkImage Image::get_api_image() {
    return image;
}

void mem::Image::init(vk::PhysicalDevice& physical_device, VkDevice& device, VkImage image, ImageData info) {
    data = info;

    Image::device = device;
    Image::phys_device = physical_device;

    Image::image = image;
    create_image_view(info.image_view_info);
}

VkImageView Image::get_api_image_view() {
    return image_view;
}

void Image::init(vk::PhysicalDevice& physical_device, VkDevice& device, ImageData info) {
    Image::device = device;
    Image::phys_device = physical_device;
    data = info;

    create_image(info.image_info);
    create_image_view(info.image_view_info);
}

void Image::create_image(ImageCreateInfo info) {
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = info.flags;
    image_info.imageType = info.imageType;
    image_info.format = info.format;
    image_info.extent = info.extent;
    image_info.mipLevels = info.mipLevels;
    image_info.arrayLayers = info.arrayLayers;
    image_info.samples = info.samples;
    image_info.tiling = info.tiling;
    image_info.usage = info.usage;
    image_info.sharingMode = info.sharingMode;
    image_info.queueFamilyIndexCount = info.queueFamilyIndexCount;
    image_info.pQueueFamilyIndices = info.pQueueFamilyIndices;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult image_create = vkCreateImage(device, &image_info, nullptr, &image);

    if (image_create != VK_SUCCESS) {
        printf("[ERROR %d] - could not create image \n", image_create);
    }

    //allocate memory
    VkMemoryRequirements memory_req{};
    vkGetImageMemoryRequirements(device, image, &memory_req);

    //create some memory for the image
    VkMemoryAllocateInfo memory_alloc{};
    memory_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc.allocationSize = memory_req.size;

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(phys_device, &memoryProperties);

    VkMemoryPropertyFlags properties = info.memoryProperties;

    uint32_t memoryIndex = 0;
    //uint32_t suitableMemoryForBuffer = 0;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if (memory_req.memoryTypeBits & (1 << i) && ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
            memoryIndex = i;
            break;
        }
    }

    memory_alloc.memoryTypeIndex = findMemoryType(phys_device, memory_req.memoryTypeBits, info.memoryProperties);

    VkResult alloc_result = vkAllocateMemory(device, &memory_alloc, nullptr, &memory);

    if (alloc_result != VK_SUCCESS) {
        msg::print_line("[ERROR - " + std::to_string(alloc_result) + "] - could not allocate memory");
    }

    if (vkBindImageMemory(device, image, memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("could not bind memory to image");
    }
}


///
/// transfers data from a buffer to an image
/// PARAMETERS
///     - mem::Memory buffer : the source buffer that the data will be transferred from
///     - mem::Memory image : the image that the data will be transferred to
///     - VkOffset3D image_offset : which part of the image the buffer data will be mapped to
///     - VkExtent3D map_size : describes the size of the image that the buffer is copying,
//                              if null, will use default size of image
/// RETURNS - VOID
void Image::copy_from_buffer(VkBuffer buffer, VkOffset3D image_offset, std::optional<VkExtent3D> map_size, VkQueue queue, VkCommandPool command_pool, std::optional<VkCommandBuffer> command_buffer) {
    //create command buffer
    bool delete_buffer = false;
    if (!command_buffer.has_value()) {
        command_buffer = begin_command_buffer(device, command_pool);
        delete_buffer = true;
    }

    VkImageSubresourceLayers image_layers{};
    image_layers.aspectMask = data.image_view_info.aspect_mask;
    image_layers.mipLevel = data.image_view_info.base_mip_level;
    image_layers.layerCount = data.image_info.arrayLayers;
    image_layers.baseArrayLayer = data.image_view_info.base_array_layer;

    //note for future me: this isn't about the size of destination image, its about the size of image in the buffer that your copying over.

    VkBufferImageCopy bufferCopy{};
    bufferCopy.bufferOffset = 0;
    bufferCopy.bufferRowLength = 0;
    bufferCopy.bufferImageHeight = 0;
    bufferCopy.imageSubresource = image_layers;
    bufferCopy.imageOffset = image_offset;
    bufferCopy.imageExtent = map_size.has_value() ? map_size.value() : data.image_info.extent;

    vkCmdCopyBufferToImage(command_buffer.value(),
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &bufferCopy
    );

    //destroy transfer buffer, shouldnt need it after copying the data.
    if (delete_buffer) {
        end_command_buffer(device, queue, command_pool, command_buffer.value());
    }
}

/// <summary>
///   Copies the entire contents of an image onto a particular place in a buffer
/// </summary>
/// <param name="buffer"> buffer the image data will be copied to </param>
/// <param name="dst_offset"> the location in the buffer where the image data is mapped to</param>
void Image::copy_to_buffer(VkBuffer buffer, VkDeviceSize dst_offset, VkQueue queue, VkCommandPool command_pool, std::optional<VkCommandBuffer> command_buffer)  {
    bool delete_buffer = false; 
    if (!command_buffer.has_value()) { 
        command_buffer = begin_command_buffer(device, command_pool);
        delete_buffer = true;
    }

    //we won't be dealing with mipmap levels for a while i think so i can change this then
    VkImageSubresourceLayers image_layers{};
    image_layers.aspectMask = data.image_view_info.aspect_mask;
    image_layers.mipLevel = data.image_view_info.base_mip_level;
    image_layers.layerCount = data.image_info.arrayLayers;
    image_layers.baseArrayLayer = data.image_view_info.base_array_layer;

    VkBufferImageCopy copy_data{};
    copy_data.bufferOffset = dst_offset;
    copy_data.bufferImageHeight = 0.0f; //ignore
    copy_data.bufferRowLength = 0.0f;
    copy_data.imageSubresource = image_layers;
    copy_data.imageOffset = VkOffset3D{ 0, 0, 0 };
    copy_data.imageExtent = VkExtent3D{ data.image_info.extent.width, data.image_info.extent.height, data.image_info.extent.depth };

    //copy image to buffer
    //TODO: contain this various image data within the memory object so the user doesnt have to keep track of it
    vkCmdCopyImageToBuffer(command_buffer.value(), image, data.image_info.initialLayout, buffer, 1, &copy_data);

    if (delete_buffer) { 
        end_command_buffer(device, queue, command_pool, command_buffer.value());
    }
}


void Image::transfer(VkImageLayout output_layout, VkQueue queue, VkCommandPool pool, std::optional<VkCommandBuffer> command_buffer) {
    //begin command buffer
    bool delete_buffer = false;
    if (!command_buffer.has_value()) {
        command_buffer = begin_command_buffer(device, pool);
        delete_buffer = true;
    }

    VkImageLayout initial_layout = data.image_info.initialLayout;
    data.image_info.initialLayout = output_layout;

    //transfer image layout
    VkImageMemoryBarrier imageTransfer{};
    imageTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageTransfer.pNext = nullptr;
    imageTransfer.oldLayout = initial_layout;
    imageTransfer.newLayout = output_layout;
    imageTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageTransfer.image = image;
    imageTransfer.subresourceRange.aspectMask = data.image_view_info.aspect_mask;
    imageTransfer.subresourceRange.baseMipLevel = 0;
    imageTransfer.subresourceRange.levelCount = 1;
    imageTransfer.subresourceRange.baseArrayLayer = 0;
    imageTransfer.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    if (output_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && initial_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
        imageTransfer.srcAccessMask = 0;
        imageTransfer.dstAccessMask = 0;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        imageTransfer.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        imageTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && initial_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        imageTransfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageTransfer.dstAccessMask = 0;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else if (output_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && initial_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
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
        end_command_buffer(device, queue, pool, command_buffer.value());
    }
}

void Image::create_image_view(ImageViewCreateInfo info) {
    VkImageViewCreateInfo view_info{};

    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.pNext = info.pNext;
    view_info.flags = info.flags;
    view_info.format = data.image_info.format;
    if (info.format.has_value()) {
        view_info.format = info.format.value();
    }
    view_info.components.r = info.r;
    view_info.components.b = info.b;
    view_info.components.g = info.g;
    view_info.components.a = info.a;

    view_info.image = image;
    
    VkImageSubresourceRange subresource{};
    subresource.aspectMask = info.aspect_mask;
    subresource.baseArrayLayer = info.base_array_layer;
    subresource.baseMipLevel = info.base_mip_level;
    subresource.layerCount = info.layer_count;
    subresource.levelCount = info.level_count;

    view_info.subresourceRange = subresource;
    view_info.viewType = info.view_type;

    VkResult view_create = vkCreateImageView(device, &view_info, nullptr, &image_view);

    if (view_create != VK_SUCCESS) {
        printf("[ERROR %d] - could not create image view \n", view_create);
    }
}


StackBuffer::StackBuffer() {}

StackBuffer::~StackBuffer() {
    //destroy the buffer
    //destroy();
}

void StackBuffer::destroy() {
    vkFreeMemory(device, inter_memory, nullptr);
    vkFreeMemory(device, buffer_memory, nullptr);
    vkDestroyBuffer(device, buffer, nullptr);
    vkDestroyBuffer(device, inter_buffer, nullptr);
}

void StackBuffer::init(vk::PhysicalDevice& physical_device, VkDevice& device, VkCommandPool& command_pool, BufferCreateInfo* p_buffer_info) {
    StackBuffer::device = device;
    StackBuffer::phys_device = physical_device;
    StackBuffer::command_pool = command_pool;
    //create buffer 
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext = p_buffer_info->pNext;
    createInfo.flags = p_buffer_info->flags;
    createInfo.usage = p_buffer_info->usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    createInfo.size = p_buffer_info->size;
    createInfo.sharingMode = p_buffer_info->sharingMode;
    createInfo.queueFamilyIndexCount = p_buffer_info->queueFamilyIndexCount;
    createInfo.pQueueFamilyIndices = p_buffer_info->pQueueFamilyIndices;

    if (vkCreateBuffer(device, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create buffer");
    };

    //allocate desired memory to buffer
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    //allocate memory for buffer
    VkMemoryAllocateInfo memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.allocationSize = memRequirements.size;
    //too lazy to even check if this exists will do later TODO
    memoryInfo.memoryTypeIndex = findMemoryType(physical_device, memRequirements.memoryTypeBits, p_buffer_info->memoryProperties);

    VkResult allocResult = vkAllocateMemory(device, &memoryInfo, nullptr, &buffer_memory);

    if (allocResult != VK_SUCCESS) {
        throw std::runtime_error("could not allocate memory for memory pool");
    }

    if (vkBindBufferMemory(device, buffer, buffer_memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("could not bind allocated memory to buffer");
    }

    buffer_size = p_buffer_info->size;
    offset = 0;

    //setup command buffer pool for defragmentation
    setup_queues();
    create_inter_buffer(INTER_BUFFER_SIZE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &inter_buffer, &inter_memory);
}


void StackBuffer::setup_queues() {
    tuco::QueueData indices(phys_device);
    transfer_family = indices.transferFamily.value();

    vkGetDeviceQueue(device, transfer_family, 0, &transfer_queue);
}

void StackBuffer::create_inter_buffer(VkDeviceSize buffer_size, VkMemoryPropertyFlags memory_properties, VkBuffer* buffer, VkDeviceMemory* memory) {
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    createInfo.size = buffer_size;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &createInfo, nullptr, buffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create buffer");
    };

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    //allocate memory for buffer
    VkMemoryAllocateInfo memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.allocationSize = memRequirements.size;
    //too lazy to even check if this exists will do later TODO
    memoryInfo.memoryTypeIndex = findMemoryType(phys_device, memRequirements.memoryTypeBits, memory_properties);

    VkResult allocResult = vkAllocateMemory(device, &memoryInfo, nullptr, memory);

    if (vkBindBufferMemory(device, *buffer, *memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("could not bind allocated memory to buffer");
    }

    if (allocResult != VK_SUCCESS) {
        LOG("could not create intermediary buffer for StackBuffer");
    }
}

///
/// This function will sort the stack buffer so that data is compacted and memory efficient,
/// in terms of complexity this functions runs at O(n) but the actual constant time moves are going to be a somewhat
/// expensive process so it would recommended to use during downtime in the application (perhaps tracking CPU/GPU usage?)\
/// 
void StackBuffer::sort() {
    //complexity: we must touch each element at least once to check if it needs to be moved
    //            after which we must perform a fairly expensive move operation thats dependent on the size of the
    //            allocation rather than the number of allocations

    //sort through allocations

    //check if ((next allocation offset) - (allocation offset + allocation size) > MINIMUM_DISTANCE)
    //MINIMUM_DISTANCE will check whether the amount of space between the two allocations is large enough to be
    //worth doing this expensive operation in the first place

    //if the above statement passes then we must transfer our data from the stack buffer into an intermediary buffer, we can
    //accomplish this with vkCmdCopyBuffer() command. once this data is in the buffer we can make a second vkCmdCopyBuffer() call 
    //that will push our data back into the main buffer at a new offset

    VkCommandBuffer transfer_buffer;
 
    VkCommandBufferAllocateInfo bufferAllocate{};
    bufferAllocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocate.commandPool = command_pool;
    bufferAllocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocate.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(device, &bufferAllocate, &transfer_buffer) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate memory for transfer buffer");
    }

    //begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(transfer_buffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("one of the command buffers failed to begin");
    }

    //copy to buffer
    
    while (true) {
        VkDeviceSize gap = 0;
        VkDeviceSize current_offset = 0;
        VkDeviceSize data_size = 0;
        bool fragmented = false;
        for (auto it = allocations.begin(); it != allocations.end(); it++) {
            VkDeviceSize space_between = (it->first + it->second) - (gap + it->second);
            if (space_between < MINIMUM_SORT_DISTANCE) {
                gap = it->first + it->second;
            } else {
                fragmented = true;
                current_offset = it->first; 
                data_size = it->second;
                break;
            }
        }
        
        if (!fragmented) {
            break;
        }

        //TODO: if buffer is not large enough, make new inter_buffer
        VkBufferCopy src_info{};
        src_info.srcOffset = current_offset;
        src_info.dstOffset = 0;
        src_info.size = data_size;

        vkCmdCopyBuffer(transfer_buffer, buffer, inter_buffer, 1, &src_info);

        //copy back to src buffer
        VkBufferCopy dst_info{};
        dst_info.srcOffset = 0;
        dst_info.dstOffset = gap;
        dst_info.size = data_size;

        vkCmdCopyBuffer(transfer_buffer, inter_buffer, buffer, 1, &dst_info);
    }

    if (vkEndCommandBuffer(transfer_buffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create succesfully end transfer buffer");
    };

    //destroy transfer buffer, shouldnt need it after copying the data.
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transfer_buffer;

    vkQueueSubmit(transfer_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(transfer_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &transfer_buffer);
}

VkDeviceSize StackBuffer::allocate(VkDeviceSize allocation_size) {
    if (allocation_size > buffer_size) {
        //not sure how to handle this case right now
        //will most likely need to create new vertex buffer, copy all the old data, and 
        //destroy the old buffer.
        
        printf("[ERROR] - COULD NOT ALLOCATE INTO BUFFER, OUT OF SPACE \n");
        return 0;
    }

    //TODO: proper check for whether enough space exists (sort if not enough) 
    buffer_size -= allocation_size;
    VkDeviceSize push_to = offset;
    offset += allocation_size;
    allocations.push_back(std::make_pair(push_to, offset));

    return push_to;
}

VkCommandBuffer StackBuffer::begin_command_buffer() {
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


void StackBuffer::end_command_buffer(VkCommandBuffer commandBuffer) {
    //end command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create succesfully end transfer buffer");
    };

    //destroy transfer buffer, shouldnt need it after copying the data.
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(transfer_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(transfer_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &commandBuffer);
}

//EFFECTS: maps a given chunk of data to this buffer and returns the memory location of where it was mapped
VkDeviceSize StackBuffer::map(VkDeviceSize data_size, void* data) {
    VkDeviceSize memory_loc = allocate(data_size);

    VkBuffer temp_buffer;
    VkDeviceMemory temp_memory;
    create_inter_buffer(data_size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &temp_buffer, &temp_memory);

    void* pData;
    if (vkMapMemory(device, temp_memory, 0, data_size, 0, &pData) != VK_SUCCESS) {
        throw std::runtime_error("could not map data to memory");
    }
    memcpy(pData, data, data_size);
    
    //map memory to buffer
    copy_buffer(temp_buffer, buffer, memory_loc, data_size);
    

    vkUnmapMemory(device, temp_memory);

    //TODO: should minimize the amount of temp buffers created, having one at a set size only recreating a new temp buffer if our previous one was too small.
    vkFreeMemory(device, temp_memory, nullptr);
    vkDestroyBuffer(device, temp_buffer, nullptr);

    return memory_loc;
}

void StackBuffer::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize dst_offset, VkDeviceSize data_size) {
    VkCommandBuffer transferBuffer = begin_command_buffer();

    //transfer between buffers
    VkBufferCopy copyData{};
    copyData.srcOffset = 0;
    copyData.dstOffset = dst_offset;
    copyData.size = data_size;

    vkCmdCopyBuffer(transferBuffer,
        src_buffer,
        dst_buffer,
        1,
        &copyData
    );

    //destroy transfer buffer, shouldnt need it after copying the data.
    end_command_buffer(transferBuffer);
}

void StackBuffer::free(VkDeviceSize delete_offset) {
    //the only thing the user would have at this point is the offset
    //this is fine for a free call, we won't actually delete the data, but by
    //deleting the pointer to that data, when we sort the data later this "freed" data
    //will be overwritten and disappear
    VkDeviceSize current_offset = 0;
    for (auto it = allocations.begin(); it != allocations.end(); it++) {
        if (delete_offset == it->first) {
            //delete at location
            allocations.erase(it);
            break;
        }
    }
}

Pool::Pool(VkDevice& device, PoolCreateInfo create_info) {
    api_device = device;
    pool_create_info = create_info;
    createPool(create_info);
}

Pool::~Pool() {
    //msg::print_line("deleting buffer...");
    //destroyPool();
}

void Pool::destroyPool() {
    for (size_t i = 0; i < pools.size(); i++) {
        vkDestroyDescriptorPool(api_device, pools[i], nullptr);
    }
}

void Pool::createPool(PoolCreateInfo create_info) {
    //create pool described by pool_info
    VkDescriptorPoolSize pool_size{};
    pool_size.type = create_info.set_type;
    pool_size.descriptorCount = create_info.pool_size;
    
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = create_info.pool_size;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    size_t current_size = pools.size();
    pools.resize(current_size + 1);
    vkCreateDescriptorPool(api_device, &pool_info, nullptr, &pools[current_size]);
    allocations.push_back(create_info.pool_size);
}

size_t Pool::allocate(VkDeviceSize allocation_size) {
    //check if we can allocate to a pool
    bool allocate = false;
    size_t pool_to_allocate = 0;
    for (size_t i = 0; i < pools.size(); i++) {
        if (allocations[i] >= allocation_size) {
            allocate = true;
            pool_to_allocate = i;
            break;
        }
    }

    if (!allocate) {
        pool_create_info.pool_size = int(std::max(std::pow(pool_create_info.pool_size, 2), (double)allocation_size));
        createPool(pool_create_info);
        pool_to_allocate = allocations.size() - 1;
    }

    //now we have a pool to allocate to
    allocations[pool_to_allocate] -= allocation_size;

    //ERROR? we might not get garbage data back from this...
    return pool_to_allocate;
}

uint32_t mem::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    //uint32_t suitableMemoryForBuffer = 0;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
            return i;
        }
    }

    throw std::runtime_error("could not find appropriate memory type");

}
//PURPOSE - create a buffer and allocate it with the memory required by the user
//PARAMETERS - [VkPhysicalDevice] physicalDevice - the GPU the renderer is rendering on
//           - [VkDevice] device - the logical device containing info on which parts of the GPU i'll be using
//           - [BufferCreateInfo*] pCreateInfo - information struct needed to create a buffer
//           - [Memory*] memory - the memory struct this function will write its data to
//RETURNS - NONE
void mem::createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, BufferCreateInfo* pCreateInfo, Memory* pMemory) {
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext = pCreateInfo->pNext;
    createInfo.flags = pCreateInfo->flags;
    createInfo.usage = pCreateInfo->usage;
    createInfo.size = pCreateInfo->size;
    createInfo.sharingMode = pCreateInfo->sharingMode;
    createInfo.queueFamilyIndexCount = pCreateInfo->queueFamilyIndexCount;
    createInfo.pQueueFamilyIndices = pCreateInfo->pQueueFamilyIndices;

    if (vkCreateBuffer(device, &createInfo, nullptr, &pMemory->buffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create buffer");
    };

    //allocate desired memory to buffer
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, pMemory->buffer, &memRequirements);

    //allocate memory for buffer
    VkMemoryAllocateInfo memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.allocationSize = memRequirements.size;
    //too lazy to even check if this exists will do later TODO
    memoryInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, pCreateInfo->memoryProperties);

    VkResult allocResult = vkAllocateMemory(device, &memoryInfo, nullptr, &pMemory->memoryHandle);

    if (allocResult != VK_SUCCESS) {
        throw std::runtime_error("could not allocate memory for memory pool");
    }

    if (vkBindBufferMemory(device, pMemory->buffer, pMemory->memoryHandle, 0) != VK_SUCCESS) {
        throw std::runtime_error("could not bind allocated memory to buffer");
    }

    //initialize the memory chunk for the maAllocateMemory to use
    Space freeSpace{};
    freeSpace.offset = 0;
    freeSpace.size = pCreateInfo->size;

    pMemory->locations.push_back(freeSpace);
}
//PURPOSE - create a image and allocate it with the memory required by the user
void mem::createImage(VkPhysicalDevice physicalDevice, VkDevice device, ImageCreateInfo* imageInfo, Memory* pMemory)  {
    //create image 
    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = imageInfo->flags;
    createInfo.imageType = imageInfo->imageType;
    createInfo.format = imageInfo->format;
    createInfo.extent = imageInfo->extent;
    createInfo.mipLevels = imageInfo->mipLevels;
    createInfo.arrayLayers = imageInfo->arrayLayers;
    createInfo.samples = imageInfo->samples;
    createInfo.tiling = imageInfo->tiling;
    createInfo.usage = imageInfo->usage;
    createInfo.sharingMode = imageInfo->sharingMode;
    createInfo.queueFamilyIndexCount = imageInfo->queueFamilyIndexCount;
    createInfo.pQueueFamilyIndices = imageInfo->pQueueFamilyIndices;
    createInfo.initialLayout = imageInfo->initialLayout;
    
    if (vkCreateImage(device, &createInfo, nullptr, &pMemory->image) != VK_SUCCESS) {
        throw std::runtime_error("could not create image");
    }

    //allocate memory
    VkMemoryRequirements memoryReq{};
    vkGetImageMemoryRequirements(device, pMemory->image, &memoryReq);

    //create some memory for the image
    VkMemoryAllocateInfo memoryAlloc{};
    memoryAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAlloc.allocationSize = memoryReq.size;

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    VkMemoryPropertyFlags properties = imageInfo->memoryProperties;

    uint32_t memoryIndex = 0;
    //uint32_t suitableMemoryForBuffer = 0;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if (memoryReq.memoryTypeBits & (1 << i) && ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
            memoryIndex = i;
            break;
        }
    }

    memoryAlloc.memoryTypeIndex = findMemoryType(physicalDevice, memoryReq.memoryTypeBits, imageInfo->memoryProperties);

    //save image dimensions we may need depth in the future but it won't be hard to add
    pMemory->imageDimensions.width = imageInfo->extent.width;
    pMemory->imageDimensions.height = imageInfo->extent.height;

    VkResult allocResult = vkAllocateMemory(device, &memoryAlloc, nullptr, &pMemory->memoryHandle);

    if (allocResult != VK_SUCCESS) {
        throw std::runtime_error("could not allocate memory for memory pool");
    }

    if (vkBindImageMemory(device, pMemory->image, pMemory->memoryHandle, 0) != VK_SUCCESS) {
        throw std::runtime_error("could not bind memory to image");
    }

}

void mem::createImageView(VkDevice device, ImageViewCreateInfo viewInfo, Memory* pMemory) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = viewInfo.pNext;
    createInfo.flags = viewInfo.flags;
    createInfo.image = pMemory->image;
    createInfo.viewType = viewInfo.view_type;
    createInfo.format = viewInfo.format.value();
    createInfo.components.r = viewInfo.r;
    createInfo.components.b = viewInfo.b;
    createInfo.components.g = viewInfo.g;
    createInfo.components.a = viewInfo.a;
    createInfo.subresourceRange.aspectMask = viewInfo.aspect_mask;
    createInfo.subresourceRange.baseArrayLayer = viewInfo.base_array_layer;
    createInfo.subresourceRange.baseMipLevel = viewInfo.base_mip_level;
    createInfo.subresourceRange.layerCount = viewInfo.layer_count;
    createInfo.subresourceRange.levelCount = viewInfo.level_count;

    if (vkCreateImageView(device, &createInfo, nullptr, &pMemory->imageView) != VK_SUCCESS) {
        throw std::runtime_error("could not create image view");
    }
}

//PURPOSE - given a memory block preserve a chunk of memory to be 'allocated' so that it can be filled with data without causing
//          conflicts
//PARAMETRS - [VkDeviceSize] allocationSize (how much memory needs to be preserved in bytes)
//            [Memory*] pMemory (memory struct which we will pass on the data for where the memory has been preserved)
//RETURNS - NONE
void mem::allocateMemory(VkDeviceSize allocationSize, Memory* pMemory, VkDeviceSize* force_offset) {
    if (force_offset != nullptr) {
        //if the user is forcing this specific place in memory then all bets are off, and the user may end up
        //overwriting data they needed.

        pMemory->allocate = true;
        pMemory->offset = *force_offset;
    }
    else {
        for (auto it = pMemory->locations.rbegin(); it != pMemory->locations.rend(); it++) {
            if (it->size >= allocationSize) {
                //printf("allocation spot size: %zu, allocation size: %zu \n", it->size, allocationSize);
                VkDeviceSize offsetLocation;
                memcpy(&offsetLocation, &(it->offset), sizeof(it->offset));
                pMemory->offset = offsetLocation;
                pMemory->allocate = true;

                it->offset = it->offset + allocationSize;
                it->size = it->size - allocationSize;

                //printf("allocation size afterwards: %zu", it->size);
                break;
            }
        }
    }

    if (!pMemory->allocate) {
        throw std::runtime_error("welp now u have to fix this");
    }
}

//PURPOSE - free memory described by the struct the user gives so that future memory allocations can use the space to allocate memory
//PARAMETERS - [MaFreeMemoryInfo] freeInfo (struct describing the data the user wants to  free)
//           - [Memory*] pMemory (pointer to the memory where the data is located)
//RETURNS - NONE
void mem::freeMemory(FreeMemoryInfo freeInfo, Memory* pMemory) {
    //need to do a check on where this data is
    //iterate through the memory locations to figure out if any offsets match
    for (auto it = pMemory->locations.begin(); it != pMemory->locations.end(); it++) {
        VkDeviceSize freeOffset = freeInfo.deleteOffset + freeInfo.deleteSize;
        if (freeOffset < it->offset) {
            //we know we need to insert a new value into the vector, so we can break after we finish
            const Space& freeSpace = {
                freeInfo.deleteOffset,
                freeInfo.deleteSize + (it->offset - freeOffset),
            };

            //insert into list
            pMemory->locations.insert(it, freeSpace);
            break;
        }
        if (freeOffset == it->offset) {
            //we know we need to move the value in the vector, so we can break after we finish
            it->offset -= freeInfo.deleteSize;
            it->size += freeInfo.deleteSize;
            break;
        }
    }
}

//PURPOSE - map the given data to the given memory
//PARAMETERS - [VkDevice] device - the logical device
//           - [VkDeviceSize] dataSize - the size of the data being mapped
//           - [Memory*] pMemory - pointer to memory struct
//           - [void*] data - the data actually being mapped
void mem::mapMemory(VkDevice device, VkDeviceSize dataSize, Memory* pMemory, void* data) {
    if (!pMemory->allocate) {
        throw std::runtime_error("tried to map memory to unallocated data");
    }
    pMemory->allocate = false;

    void* pData;
    if (vkMapMemory(device, pMemory->memoryHandle, pMemory->offset, dataSize, 0, &pData) != VK_SUCCESS) {
        throw std::runtime_error("could not map data to memory");
    }
    memcpy(pData, data, dataSize);
    vkUnmapMemory(device, pMemory->memoryHandle);
}

void mem::destroyBuffer(VkDevice device, Memory maMemory) {
    vkDeviceWaitIdle(device);
    //free the large chunk of memory
    vkFreeMemory(device, maMemory.memoryHandle, nullptr);

    //destroy the associated buffer
    vkDestroyBuffer(device, maMemory.buffer, nullptr);
}

void mem::destroyImage(VkDevice device, Memory maMemory) {
    vkDeviceWaitIdle(device);
    //free the large chunk of memory
    vkFreeMemory(device, maMemory.memoryHandle, nullptr);

    //destroy the associated image
    vkDestroyImageView(device, maMemory.imageView, nullptr); //should have check here to see if image view exists...
    vkDestroyImage(device, maMemory.image, nullptr);
}
