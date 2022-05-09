/* ----------------------- memory_allocator.hpp ----------------------
 * Handles memory allocations of vulkan objects (images, buffers, etc)
 * -------------------------------------------------------------------
*/
#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>
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

class Image {

};

class SearchBuffer {
private:
    std::vector<VkDeviceSize> memory_locations;
    VkDeviceSize memory_offset;
    VkDeviceMemory buffer_memory;
public:
    VkBuffer buffer;
public:
    SearchBuffer();
    ~SearchBuffer();
public:
    void init(VkPhysicalDevice physical_device, VkDevice device, BufferCreateInfo* p_buffer_info);
    void destroy(VkDevice device);
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
    std::shared_ptr<VkDevice> p_device; //hopefully this doesn't just disappear...
    std::shared_ptr<VkPhysicalDevice> p_phys_device;
    std::shared_ptr<VkCommandPool> p_command_pool;
    //pair with allocation size and offset? 
    std::vector<std::pair<VkDeviceSize, VkDeviceSize>> allocations;

    VkQueue transfer_queue;
    uint32_t transfer_family;

    VkBuffer inter_buffer;
    VkDeviceMemory inter_memory;

public:
    VkBuffer buffer;

public:
    StackBuffer();
    ~StackBuffer();

public:
    void init(VkPhysicalDevice* physical_device, VkDevice* device, VkCommandPool* command_pool, BufferCreateInfo* p_buffer_info);
    VkDeviceSize map(VkDeviceSize data_size, void* data);
    void destroy(VkDevice device);
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


public:
    Pool(VkDevice device, PoolCreateInfo create_info);
    ~Pool();
public:   
    std::vector<VkDescriptorPool> pools;
    
    size_t allocate(VkDevice device, VkDeviceSize allocationSize);
    void destroyPool(VkDevice device);
    void createPool(VkDevice device, PoolCreateInfo create_info);
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

struct ImageCreateInfo {
    VkImageCreateFlags flags = 0;
    VkImageType imageType;
    VkFormat format;
    VkExtent3D extent;
    uint32_t mipLevels;
    uint32_t arrayLayers;
    VkSampleCountFlagBits samples;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkImageLayout initialLayout;
    VkDeviceSize size;
    VkSharingMode sharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;

    VkMemoryPropertyFlags memoryProperties;
};

struct ImageViewCreateInfo {
    void* pNext = nullptr;
    VkImageViewCreateFlags flags = 0;
    VkImageViewType viewType;
    VkFormat format;
    VkComponentMapping components;
    VkImageSubresourceRange subresourceRange;
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
