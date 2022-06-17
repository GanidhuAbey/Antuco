/* ----------------------- memory_allocator.hpp ----------------------
 * Handles memory allocations of vulkan objects (images, buffers, etc)
 * -------------------------------------------------------------------
*/
#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

//#include "engine_graphics.hpp"

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

namespace mem {

struct BufferCreateInfo {
    const void* pNext = nullptr;
    VkBufferCreateFlags flags = 0;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;
    VkMemoryPropertyFlags memoryProperties;
};


struct ImageCreateInfo {
    VkImageCreateFlags flags = 0;   
    VkImageType imageType = VK_IMAGE_TYPE_2D;
    VkFormat format;
    VkExtent3D extent;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags usage;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkDeviceSize size;
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;

    VkMemoryPropertyFlags memoryProperties;
};

struct ImageViewCreateInfo {
    void* pNext = nullptr;
    VkImageViewCreateFlags flags = 0;
    VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
    std::optional<VkFormat> format = std::nullopt;
    
    VkComponentSwizzle r = VK_COMPONENT_SWIZZLE_IDENTITY;
    VkComponentSwizzle b = VK_COMPONENT_SWIZZLE_IDENTITY;
    VkComponentSwizzle g = VK_COMPONENT_SWIZZLE_IDENTITY;
    VkComponentSwizzle a = VK_COMPONENT_SWIZZLE_IDENTITY;
    
    //image subresource
    uint32_t layer_count = 1;
    uint32_t base_array_layer = 0;
    uint32_t level_count = 1;
    uint32_t base_mip_level = 0;
    VkImageAspectFlags aspect_mask;
};

struct ImageData {
    std::string name = "null";
    ImageCreateInfo image_info;
    ImageViewCreateInfo image_view_info;
};

class Image {
private:
    VkDevice device;
    VkPhysicalDevice phys_device;

    ImageData data;

    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
public:
    Image();
    ~Image();
    void init(vk::PhysicalDevice& physical_device, VkDevice& device, ImageData info);
    void init(vk::PhysicalDevice& physical_device, VkDevice& device, VkImage image, ImageData info);
    //can only be called after init()
    void destroy();
    void destroy_image_view();
    VkImage get_api_image();
    VkImageView get_api_image_view();

    void transfer(VkImageLayout output_layout, VkQueue queue, VkCommandPool pool, std::optional<VkCommandBuffer> command_buffer = std::nullopt);
    void copy_to_buffer(VkBuffer buffer, VkDeviceSize dst_offset, VkQueue queue, VkCommandPool command_pool, std::optional<VkCommandBuffer> command_buffer = std::nullopt);
    void copy_from_buffer(VkBuffer buffer, VkOffset3D image_offset, std::optional<VkExtent3D> map_size, VkQueue queue, VkCommandPool command_pool, std::optional<VkCommandBuffer> command_buffer = std::nullopt);

private:
    void create_image(ImageCreateInfo info);
    void create_image_view(ImageViewCreateInfo info);
};

class SearchBuffer {
private:
    std::vector<VkDeviceSize> memory_locations;
    VkDeviceSize memory_offset;
    VkDeviceMemory buffer_memory;

    VkDevice api_device;
public:
    VkBuffer buffer;
public:
    SearchBuffer();
    ~SearchBuffer();
public:
    void init(VkPhysicalDevice physical_device, VkDevice& device, BufferCreateInfo* p_buffer_info);
    void destroy();
    void write(VkDevice device, VkDeviceSize offset, VkDeviceSize data_size, void* p_data);
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
    VkCommandPool command_pool;
    //pair with allocation size and offset? 
    std::vector<std::pair<VkDeviceSize, VkDeviceSize>> allocations;

    VkQueue transfer_queue;
    uint32_t transfer_family;

    VkBuffer inter_buffer;
    VkDeviceMemory inter_memory;

    VkDevice device;
    VkPhysicalDevice phys_device;

public:
    VkBuffer buffer;

public:
    StackBuffer();
    ~StackBuffer();

public:
    void init(vk::PhysicalDevice& physical_device, VkDevice& device, VkCommandPool& command_pool, BufferCreateInfo* p_buffer_info);
    VkDeviceSize map(VkDeviceSize data_size, void* data);
    void destroy();
    void free(VkDeviceSize delete_offset);
    void sort();
    
private: 
    void setup_queues();
    void create_inter_buffer(VkDeviceSize buffer_size, VkMemoryPropertyFlags memory_properties, VkBuffer* buffer, VkDeviceMemory* memory);
    VkDeviceSize allocate(VkDeviceSize allocation_size);
    VkCommandBuffer begin_command_buffer();
    void end_command_buffer(VkCommandBuffer command_buffer);
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
    
    VkDevice api_device;


public:
    Pool(VkDevice& device, PoolCreateInfo create_info);
    ~Pool();
public:   
    std::vector<VkDescriptorPool> pools;
    
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



void createObject();
void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, BufferCreateInfo* pCreateInfo, Memory* memory);
void createImage(VkPhysicalDevice physicalDevice, VkDevice device, ImageCreateInfo* imageInfo, Memory* pMemory);
void createImageView(VkDevice device, ImageViewCreateInfo viewInfo, Memory* pMemory);
void mapMemory(VkDevice device, VkDeviceSize dataSize, Memory* pMemory, void* data);
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void createMemory(VkPhysicalDevice physicalDevice, VkDevice device, MemoryInfo* poolInfo, VkBuffer* buffer, Memory* maMemory);
void allocateMemory(VkDeviceSize allocationSize, Memory* pMemory, VkDeviceSize* force_offset=nullptr);
void freeMemory(FreeMemoryInfo freeInfo, Memory* pMemory);
void destroyBuffer(VkDevice device, Memory maMemory);
void destroyImage(VkDevice device, Memory maMemory);

} // namespace mem
