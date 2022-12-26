/* ----------------------- memory_allocator.hpp ----------------------
 * Handles memory allocations of vulkan objects (images, buffers, etc)
 * -------------------------------------------------------------------
*/
#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "vulkan_wrapper/device.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <math.h>
#include <memory>
#include <algorithm>
#include <list>
#include <utility>
#include <optional>

const VkDeviceSize INTER_BUFFER_SIZE = 1e8;
const VkDeviceSize MINIMUM_SORT_DISTANCE = 1e5;

//100 mb of data per allocation
//TODO: need to create a information struct in the style of vulkan to let user manipulate this data
//VkDeviceSize allocationSize = 5e8;

//TODO: seperate memory allocator into multiple classes

namespace mem {

struct BufferCreateInfo {
    const void* pNext = nullptr;
    vk::BufferCreateFlags flags = {};
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    vk::SharingMode sharing_mode = vk::SharingMode::eExclusive;
    uint32_t queue_family_index_count;
    const uint32_t* p_queue_family_indices;
    vk::MemoryPropertyFlags memory_properties;
};


struct ImageCreateInfo {
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

//buffer visible to the CPU, used for mapping data from the CPU to the GPU.
//amount of memory visible to both CPU and GPU is limited in systems so should only be
//used temporary cases.
class CPUBuffer {
private:
    vk::Buffer buffer;
    vk::DeviceMemory memory;

    v::Device* device;
    
public:
    void init(v::PhysicalDevice& physical_device, v::Device& device, BufferCreateInfo& buffer_info);
    void map(vk::DeviceSize size, const void* data);
    void destroy();

    vk::Buffer& get() { return buffer; }
};

class SearchBuffer {
private:
    std::vector<VkDeviceSize> memory_locations;
    vk::DeviceSize memory_offset;
    vk::DeviceMemory buffer_memory;

    v::Device* api_device;
public:
    vk::Buffer buffer;
public:
    void init(
        v::PhysicalDevice& physical_device, 
        v::Device& device, 
        BufferCreateInfo& buffer_info);

    void destroy();
    void write(
        VkDevice device, 
        VkDeviceSize offset, 
        VkDeviceSize data_size, 
        void* p_data);

    VkDeviceSize allocate(VkDeviceSize allocation_size);
    void free(VkDeviceSize offset);
};

//Buffer that sorts itself to maintain constant time allocations.
class StackBuffer {
private:
    VkDeviceSize buffer_size;
    VkDeviceSize offset;
    VkDeviceMemory buffer_memory;
    //i need a shared pointer of the device...
    vk::CommandPool command_pool;
    //pair with allocation size and offset? 
    std::vector<std::pair<VkDeviceSize, VkDeviceSize>> allocations;

    vk::Queue transfer_queue;
    uint32_t transfer_family;

    vk::Buffer inter_buffer;
    vk::DeviceMemory inter_memory;

    v::Device* device;
    v::PhysicalDevice* phys_device;

public:
    vk::Buffer buffer;

public:
    void init(
        v::PhysicalDevice& physical_device, 
        v::Device& device, 
        BufferCreateInfo& p_buffer_info);

    VkDeviceSize map(VkDeviceSize data_size, void* data);
    void destroy();
    void free(VkDeviceSize delete_offset);
    void sort();
    
private: 
    void setup_queues();
    void create_inter_buffer(vk::DeviceSize buffer_size, vk::MemoryPropertyFlags memory_properties, vk::Buffer& buffer, vk::DeviceMemory& memory);
    VkDeviceSize allocate(VkDeviceSize allocation_size);
    void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize dst_offset, VkDeviceSize data_size);
};

struct PoolCreateInfo {
    uint32_t pool_size;
    VkDescriptorType set_type; 
};

//there are more functions that the pool class will need, but since i don't need them yet,
//and hence won't know exactly what i want from them, ill leave them unfinished for now.
class Pool {
private:
    //we'll have to create many different types of pools, but for now
    //since we already know exactly which pools we want we'll just include them and made the system expandable.

    //handle a vector of all the pools we'll be allocating
    std::vector<uint32_t> allocations;
    PoolCreateInfo pool_create_info;
    
    v::Device* api_device;


public:
    Pool(v::Device& device, PoolCreateInfo create_info);
public:   
    std::vector<vk::DescriptorPool> pools;
    
    size_t allocate(VkDeviceSize allocationSize);
    void destroyPool();
    void createPool(PoolCreateInfo create_info);
    void reset();
    void freeDescriptorSets();
};

struct MemoryInfo {
    VkDeviceSize bufferSize;
    VkDeviceSize allocationSize;
    VkBufferUsageFlags bufferUsage;
    VkMemoryPropertyFlags memoryProperties;
    uint32_t queueFamilyIndexCount;

};

struct Space {
    VkDeviceSize offset;
    VkDeviceSize size;
};

struct FreeMemoryInfo {
    VkDeviceSize deleteSize;
    VkDeviceSize deleteOffset;
};

struct ImageSize {
    uint32_t width;
    uint32_t height;
};

struct Memory {
    VkBuffer buffer;
    VkImage image;
    VkImageView imageView;
    ImageSize imageDimensions;
    VkDeviceMemory memoryHandle;
    VkDeviceSize offset;
    size_t deleteIndex;
    bool allocate;

    std::list<Space> locations;
};

struct MemoryData {
    VkDeviceMemory memoryHandle;
    VkDeviceSize offset;
    VkDeviceSize resourceSize;
    size_t offsetIndex;
};



uint32_t findMemoryType(
    v::PhysicalDevice& physical_device, 
    uint32_t typeFilter, 
    vk::MemoryPropertyFlags properties);

} // namespace mem
