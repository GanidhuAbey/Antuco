/* ----------------------- memory_allocator.hpp ----------------------
 * Handles memory allocations of vulkan objects (images, buffers, etc)
 * -------------------------------------------------------------------
 */
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan_wrapper/device.hpp"

#include <list>
#include <optional>
#include <utility>
#include <vector>
#include <memory>

const VkDeviceSize INTER_BUFFER_SIZE = 1e8;
const VkDeviceSize MINIMUM_SORT_DISTANCE = 1e5;

// 100 mb of data per allocation
// TODO: need to create a information struct in the style of vulkan to let user
// manipulate this data VkDeviceSize allocationSize = 5e8;

namespace mem {


//class Resource
//{
//private:
//    bool free_resource = false;
//public:
//    Resource() = default;
//    ~Resource() = default;
//    // every resource must implement a method to clean up it's memory.
//    virtual void destroy() = 0;
//
//    void set_free(bool free) { free_resource = free; }
//    bool get_free_resource() { return free_resource; }
//
//};
//
//
//// A singleton class designed to take ownership of destroying resources when they are no longer needed.
//// TODO - for now we will only implement functionality to wipe memory at the end of program, other functionality can be added
////        as needed.
//class GarbageCollector
//{
//private:
//    static std::unique_ptr<GarbageCollector> instance;
//    GarbageCollector() = default;
//
//    std::vector<std::unique_ptr<Resource>> all_resources;
//public:
//    static GarbageCollector* get();
//
//    Resource* register_resource(Resource& resource);
//
//    ~GarbageCollector();
//};

struct BufferCreateInfo {
  const void *pNext = nullptr;
  vk::BufferCreateFlags flags = {};
  vk::DeviceSize size;
  vk::BufferUsageFlags usage;
  vk::SharingMode sharing_mode = vk::SharingMode::eExclusive;
  uint32_t queue_family_index_count;
  const uint32_t *p_queue_family_indices;
  vk::MemoryPropertyFlags memory_properties;
};

// buffer visible to the CPU, used for mapping data from the CPU to the GPU.
// amount of memory visible to both CPU and GPU is limited in systems so should
// only be used temporary cases.
class CPUBuffer {
private:
  vk::Buffer buffer;
  vk::DeviceMemory memory;

  v::Device *device;

public:
  void init(v::PhysicalDevice &physical_device, v::Device &device,
            BufferCreateInfo &buffer_info);
  void map(vk::DeviceSize size, vk::DeviceSize offset, const void *data);
  void destroy();

  vk::Buffer &get() { return buffer; }
};

class SearchBuffer {
private:
  std::vector<VkDeviceSize> memory_locations;
  vk::DeviceSize memory_offset;
  vk::DeviceMemory buffer_memory;

  v::Device *api_device;

public:
  vk::Buffer buffer;

public:
  void init(v::PhysicalDevice &physical_device, v::Device &device,
            BufferCreateInfo &buffer_info);

  void destroy();
  void writeLocal(VkDevice device, VkDeviceSize offset, VkDeviceSize data_size,
                  void *p_data);

  void writeDevice(VkDevice device, VkDeviceSize srcOffset,
                   VkDeviceSize dstOffset, VkDeviceSize dataSize,
                   VkBuffer srcBuffer, VkCommandBuffer cmdBuffer);

  VkDeviceSize allocate(VkDeviceSize allocation_size, uint32_t alignment = 1);
  void free(VkDeviceSize offset);
};

// Buffer that sorts itself to maintain constant time allocations.
class StackBuffer {
private:
  VkDeviceSize buffer_size;
  VkDeviceSize offset;
  VkDeviceMemory buffer_memory;
  // i need a shared pointer of the device...
  vk::CommandPool command_pool;
  // pair with allocation size and offset?
  std::vector<std::pair<VkDeviceSize, VkDeviceSize>> allocations;

  vk::Queue transfer_queue;
  uint32_t transfer_family;

  vk::Buffer inter_buffer;
  vk::DeviceMemory inter_memory;

  v::Device *device;
  v::PhysicalDevice *phys_device;

public:
  vk::Buffer buffer;

public:
  void init(v::PhysicalDevice &physical_device, v::Device &device,
            BufferCreateInfo &p_buffer_info);

  VkDeviceSize map(VkDeviceSize data_size, void *data);
  void destroy();
  void free(VkDeviceSize delete_offset);
  void sort();

private:
  void setup_queues();
  void create_inter_buffer(vk::DeviceSize buffer_size,
                           vk::MemoryPropertyFlags memory_properties,
                           vk::Buffer &buffer, vk::DeviceMemory &memory);
  VkDeviceSize allocate(VkDeviceSize allocation_size);
  void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer,
                  VkDeviceSize dst_offset, VkDeviceSize data_size);
};

struct PoolCreateInfo {
  uint32_t pool_size;
  VkDescriptorType set_type;
};

// there are more functions that the pool class will need, but since i don't
// need them yet, and hence won't know exactly what i want from them, ill leave
// them unfinished for now.
class Pool {
private:
  // we'll have to create many different types of pools, but for now
  // since we already know exactly which pools we want we'll just include them
  // and made the system expandable.

  // handle a vector of all the pools we'll be allocating
  std::vector<uint32_t> allocations;
  std::vector<PoolCreateInfo> poolInfo;

  std::shared_ptr<v::Device> api_device;

public:
  Pool(std::shared_ptr<v::Device> device, std::vector<PoolCreateInfo> info);
  ~Pool();

  void allocateDescriptorSets(v::Device &device, uint32_t setCount,
                              VkDescriptorSetLayout layout,
                              VkDescriptorSet *sets);

public:
  std::vector<VkDescriptorPool> pools;

  size_t allocate(VkDeviceSize allocationSize);
  void destroyPool();
  void createPool();
  void reset();
  void freeDescriptorSets();

private:
  VkDescriptorPool &getCurrentPool();
  VkResult attemptAllocation(v::Device &device, uint32_t setCount,
                             VkDescriptorSetLayout layout,
                             VkDescriptorSet *sets);
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

uint32_t findMemoryType(v::PhysicalDevice &physical_device, uint32_t typeFilter,
                        vk::MemoryPropertyFlags properties);

} // namespace mem
