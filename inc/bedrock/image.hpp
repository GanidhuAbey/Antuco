#pragma once

#include <optional>

// [TODO] - Convert to vulkan.h code.
#include <vulkan/vulkan.hpp>

#include <vulkan_wrapper/physical_device.hpp>
#include <vulkan_wrapper/device.hpp>

#include <memory_allocator.hpp>

namespace br
{

struct ImageCreateInfo
{
    vk::ImageCreateFlagBits flags = {};
    vk::ImageType image_type = vk::ImageType::e2D;
    vk::Format format;
    vk::Extent3D extent;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
    vk::ImageUsageFlags usage;
    vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;
    vk::DeviceSize size;
    vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;

    vk::MemoryPropertyFlags memory_properties;
};

struct ImageViewCreateInfo
{
    void* pNext = nullptr;
    vk::ImageViewCreateFlags flags = {};
    vk::ImageViewType view_type = vk::ImageViewType::e2D;
    std::optional<vk::Format> format = std::nullopt;

    vk::ComponentSwizzle r = vk::ComponentSwizzle::eIdentity;
    vk::ComponentSwizzle b = vk::ComponentSwizzle::eIdentity;
    vk::ComponentSwizzle g = vk::ComponentSwizzle::eIdentity;
    vk::ComponentSwizzle a = vk::ComponentSwizzle::eIdentity;

    // image subresource
    uint32_t layer_count = 1;
    uint32_t base_array_layer = 0;
    uint32_t level_count = 1;
    uint32_t base_mip_level = 0;
    vk::ImageAspectFlags aspect_mask;
};

struct ImageData
{
    std::string name = "null";
    ImageCreateInfo image_info;
    ImageViewCreateInfo image_view_info;
};

struct RawImageData
{
    unsigned char* image_data;
    int width;
    int height;
    int channels; // how many 8 bit components each pixel has (e.g RGB texture has 3*4 = 12 8 bit components)

    uint32_t buffer_size;
};

class Image
{
private:
    v::Device* device;
    v::PhysicalDevice* phys_device;

    ImageData data;

    vk::Image image;
    vk::ImageView image_view;
    VkDeviceMemory memory;

    vk::CommandPool command_pool;

    bool write = false;

    RawImageData raw_image;

public:
    void init();
    //! loads color image from filepath, creating device accessible image. (assumes that Antuco graphics already initialized).
    void load_color_image(std::string file_path);
    // [TODO] - remove references to and delete (deprecated)
    void init(v::PhysicalDevice& physical_device, v::Device& device,
                ImageData info);
    void init(v::PhysicalDevice& physical_device, v::Device& device,
                VkImage image, ImageData info);
    // can only be called after init()
    void destroy();
    void destroy_image_view();
    vk::Image get_api_image();
    vk::ImageView get_api_image_view();

    void transfer(vk::ImageLayout output_layout, vk::Queue queue,
                    std::optional<vk::CommandBuffer> command_buffer = std::nullopt,
                    vk::ImageLayout current_layout = vk::ImageLayout::eUndefined);

    void copy_to_buffer(
        vk::Buffer buffer, VkDeviceSize dst_offset, vk::Queue queue,
        std::optional<vk::CommandBuffer> command_buffer = std::nullopt);

    void copy_from_buffer(
        vk::Buffer buffer, vk::Offset3D image_offset,
        std::optional<vk::Extent3D> map_size, vk::Queue queue,
        std::optional<vk::CommandBuffer> command_buffer = std::nullopt);

    void set_write(bool write) { Image::write = write; }

private:
    void create_image();
    void create_image_view();

    void copy_to_buffer(RawImageData &data, mem::CPUBuffer* buffer);
};


}