#pragma once

namespace v {

struct ImageViewCreateInfo {
    void* pNext = nullptr;
    vk::ImageViewCreateFlags flags = {};
    vk::ImageViewType view_type = vk::ImageViewType::e2D;
    std::optional<vk::Format> format = std::nullopt;

    vk::ComponentSwizzle r = vk::ComponentSwizzle::eIdentity;
    vk::ComponentSwizzle b = vk::ComponentSwizzle::eIdentity;
    vk::ComponentSwizzle g = vk::ComponentSwizzle::eIdentity;
    vk::ComponentSwizzle a = vk::ComponentSwizzle::eIdentity;

    //image subresource
    uint32_t layer_count = 1;
    uint32_t base_array_layer = 0;
    uint32_t level_count = 1;
    uint32_t base_mip_level = 0;
    vk::ImageAspectFlags aspect_mask;
};

struct ImageData {
    std::string name = "null";
    ImageCreateInfo image_info;
    ImageViewCreateInfo image_view_info;
};

class Image {
    class Image {
    private:
        v::Device* device;
        v::PhysicalDevice* phys_device;

        ImageData data;

        vk::Image image;
        vk::ImageView image_view;
        VkDeviceMemory memory;

        vk::CommandPool command_pool;

        bool write = false;
    public:
        void init(v::PhysicalDevice& physical_device, v::Device& device, ImageData info);
        void init(v::PhysicalDevice& physical_device, v::Device& device, VkImage image, ImageData info);
        //can only be called after init()
        void destroy();
        void destroy_image_view();
        vk::Image get_api_image();
        vk::ImageView get_api_image_view();

        vk::ImageLayout get_layout() {
            return data.image_info.initial_layout;
        }

        void transfer(
            vk::ImageLayout output_layout,
            vk::Queue queue,
            std::optional<vk::CommandBuffer> command_buffer = std::nullopt,
            vk::ImageLayout current_layout = vk::ImageLayout::eUndefined);

        void copy_to_buffer(
            vk::Buffer buffer,
            VkDeviceSize dst_offset,
            vk::Queue queue,
            std::optional<vk::CommandBuffer> command_buffer = std::nullopt);

        void copy_from_buffer(
            vk::Buffer buffer,
            vk::Offset3D image_offset,
            std::optional<vk::Extent3D> map_size,
            vk::Queue queue,
            std::optional<vk::CommandBuffer> command_buffer = std::nullopt);

        void set_write(bool write) { Image::write = write; }

    private:
        void create_image(ImageCreateInfo info);
        void create_image_view(ImageViewCreateInfo info);
    };
};

}