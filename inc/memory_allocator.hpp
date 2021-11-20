#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

//#include "engine_graphics.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <math.h>
#include <algorithm>
#include <list>

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

//keep a hash table of data, with a key being the offset, and the data allocated being the item
//when a allocation is called, check if the allocation can fit, and if it can return offset to the user.
//add offset to key in hash with copy of data being allocated. and map the data to the buffer

//when the user calls free, they will provide the offset, use offset as key to search hashtable and find data at that location
//if we store the offsets in an array, and then attach the index of that array to the item, then we can find the location ofthe offset,
//and then map all the data above the offset till we close the gap.

//at minimum we're at O(n) deallocation time, but we don't really know how costly it its to remap data, in theory since we're aren't dealing the operating
//system when we do this it should be fast, and since it it doesn't grow with the data given we can consider it as constant time
class StackBuffer {
private:
    VkDeviceSize buffer_size;
    VkDeviceSize offset;
    VkDeviceMemory buffer_memory;
    VkDevice* p_device; //hopefully this doesn't just disappear...
    //offset = i * allocations[i]   
    std::vector<VkDeviceSize> allocations;

public:
    VkBuffer buffer;

public:
    StackBuffer();
    ~StackBuffer();

public:
    void init(VkPhysicalDevice physical_device, VkDevice device, BufferCreateInfo* p_buffer_info);
    VkDeviceSize allocate(VkDeviceSize allocation_size);
    void free(VkDeviceSize delete_offset);
    void sort();
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
