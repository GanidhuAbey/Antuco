#include "memory_allocator.hpp"

#include <bitset>
#include <cstring>
using namespace mem;



Pool::Pool(VkDevice device, PoolCreateInfo create_info) {
    pool_create_info = create_info;
    createPool(device, create_info);
}

Pool::~Pool() {
}

void Pool::createPool(VkDevice device, PoolCreateInfo create_info) {
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
    vkCreateDescriptorPool(device, &pool_info, nullptr, &pools[current_size]);
    allocations.push_back(create_info.pool_size);
}

void Pool::allocate(VkDevice device, VkDescriptorPool* pool) {
    //check if we can allocate to a pool
    bool allocate = false;
    size_t pool_to_allocate = 0;
    for (size_t i = 0; i < pools.size(); i++) {
        if (allocations[i] > 0) {
            allocate = true;
            pool_to_allocate = i;
            break;
        }
    }

    if (!allocate) {
        createPool(device, pool_create_info);
        pool_to_allocate = allocations.size() - 1;
    }

    //now we have a pool to allocate to
    allocations[pool_to_allocate] -= 1;

    //ERROR? we might not get garbage data back from this...
    pool = &pools[pool_to_allocate];
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
    createInfo.viewType = viewInfo.viewType;
    createInfo.format = viewInfo.format;
    createInfo.components = viewInfo.components;
    createInfo.subresourceRange = viewInfo.subresourceRange;

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