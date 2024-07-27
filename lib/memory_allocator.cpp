#include "memory_allocator.hpp"
#include "api_config.hpp"
#include "logger/interface.hpp"
#include "queue.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan_wrapper/limits.hpp"

#include <cstring>
#include <stdexcept>

using namespace mem;

//std::unique_ptr<GarbageCollector> GarbageCollector::instance;
//
//GarbageCollector* GarbageCollector::get()
//{
//    if (!instance) 
//    {
//        instance = std::make_unique<GarbageCollector>();
//    }
//    return instance.get();
//}
//
//GarbageCollector::~GarbageCollector()
//{
//    // on de-initialization clear all resources that have not been freed yet.
//    for (auto& resource : all_resources)
//    {
//        resource->destroy();
//    }
//}
//
//// TODO: does pointer make sense here, if what provided the pointer disappears, then the garbage collector would lose it's handle
////       to the resource.
//
//// Perhaps the garbage collector should have ownership of the resource, and it should provide pointers to everyone else?
//// As short term solution, we can just use a copy, since the vulkan objects are all pointers, they will never get copied along with the object.
//Resource* GarbageCollector::register_resource(Resource& resource)
//{
//    uint32_t index = all_resources.size();
//    all_resources.push_back(std::make_unique<Resource>(resource));
//    // Perhaps we can have a wrapper around our resource classes to handle registration and hide pointers?
//    return all_resources[index].get();
//}

void SearchBuffer::init(v::PhysicalDevice &physical_device, v::Device &device,
                        BufferCreateInfo &buffer_info) 
{
    api_device = &device;

    auto create_info = vk::BufferCreateInfo(
        {}, buffer_info.size, buffer_info.usage, buffer_info.sharing_mode,
        buffer_info.queue_family_index_count, buffer_info.p_queue_family_indices);

    buffer = device.get().createBuffer(create_info);

    auto mem_req = device.get().getBufferMemoryRequirements(buffer);

    auto mem_info = vk::MemoryAllocateInfo(
        mem_req.size, findMemoryType(physical_device, mem_req.memoryTypeBits,
                                    buffer_info.memory_properties));

    buffer_memory = device.get().allocateMemory(mem_info);

    device.get().bindBufferMemory(buffer, buffer_memory, 0);

    memory_offset = 0;
}

void SearchBuffer::destroy() 
{
    api_device->get().waitIdle();
    api_device->get().free(buffer_memory);
    api_device->get().destroyBuffer(buffer);
}

VkDeviceSize SearchBuffer::allocate(VkDeviceSize allocation_size, uint32_t alignment /* = 1 */) 
{
    VkDeviceSize offset = memory_offset;
    memory_locations.push_back(offset);

    memory_offset += allocation_size;
    if (allocation_size % alignment != 0)
    {
        memory_offset += (alignment - allocation_size % alignment);
    }

    return offset;
}

// TODO: this type of allocation will only work if the search buffer is host
// visible.
void SearchBuffer::writeLocal(VkDevice device, VkDeviceSize offset,
                              VkDeviceSize data_size, void *p_data) 
{
    void *pData;
    if (vkMapMemory(device, buffer_memory, offset, data_size, 0, &pData) !=
        VK_SUCCESS) 
    {
    throw std::runtime_error("could not map data to memory");
    }
    memcpy(pData, p_data, data_size);
    vkUnmapMemory(device, buffer_memory);
}

void SearchBuffer::writeDevice(VkDevice device, VkDeviceSize srcOffset,
                               VkDeviceSize dstOffset, VkDeviceSize dataSize,
                               VkBuffer srcBuffer, VkCommandBuffer cmdBuffer) {
    // transfer between buffers
    VkBufferCopy copyData{};
    copyData.srcOffset = srcOffset;
    copyData.dstOffset = dstOffset;
    copyData.size = dataSize;

    vkCmdCopyBuffer(cmdBuffer, srcBuffer, buffer, 1, &copyData);
}

// this almost never needed when dealing with uniform buffer, so i'm not gonna
// write this function for now
void SearchBuffer::free(VkDeviceSize offset) {}

void CPUBuffer::destroy() {
    device->get().freeMemory(memory);
    device->get().destroyBuffer(buffer);
}

void CPUBuffer::init(v::PhysicalDevice &physical_device, v::Device &device,
                     BufferCreateInfo &buffer_info) 
{
    CPUBuffer::device = &device;

    auto create_info = vk::BufferCreateInfo(
        {}, buffer_info.size, buffer_info.usage, buffer_info.sharing_mode,
        buffer_info.queue_family_index_count, buffer_info.p_queue_family_indices);

    buffer = device.get().createBuffer(create_info);

    auto mem_req = device.get().getBufferMemoryRequirements(buffer);

    auto mem_info = vk::MemoryAllocateInfo(
        mem_req.size, findMemoryType(physical_device, mem_req.memoryTypeBits,
                                    vk::MemoryPropertyFlagBits::eHostVisible));

    memory = device.get().allocateMemory(mem_info);

    device.get().bindBufferMemory(buffer, memory, 0);
}

void CPUBuffer::map(vk::DeviceSize size, const void *data) 
{
    auto p_data = device->get().mapMemory(memory, 0, size);
    memcpy(p_data, data, size);
    device->get().unmapMemory(memory);
}

void StackBuffer::destroy() {
  device->get().free(inter_memory);
  device->get().free(buffer_memory);
  device->get().destroyBuffer(buffer);
  device->get().destroyBuffer(inter_buffer);
  device->get().destroyCommandPool(command_pool);
}

void StackBuffer::init(v::PhysicalDevice &physical_device, v::Device &device,
                       BufferCreateInfo &buffer_info) {

  StackBuffer::device = &device;
  StackBuffer::phys_device = &physical_device;

  StackBuffer::command_pool =
      tuco::create_command_pool(device, device.get_transfer_family());

  auto create_info = vk::BufferCreateInfo(
      {}, buffer_info.size, buffer_info.usage, buffer_info.sharing_mode,
      buffer_info.queue_family_index_count, buffer_info.p_queue_family_indices);

  buffer = device.get().createBuffer(create_info);

  auto mem_req = device.get().getBufferMemoryRequirements(buffer);

  auto mem_info = vk::MemoryAllocateInfo(
      mem_req.size, findMemoryType(physical_device, mem_req.memoryTypeBits,
                                   buffer_info.memory_properties));

  buffer_memory = device.get().allocateMemory(mem_info);

  device.get().bindBufferMemory(buffer, buffer_memory, 0);

  buffer_size = buffer_info.size;
  offset = 0;

  // setup command buffer pool for defragmentation
  setup_queues();

  create_inter_buffer(INTER_BUFFER_SIZE,
                      vk::MemoryPropertyFlagBits::eDeviceLocal, inter_buffer,
                      inter_memory);
}

void StackBuffer::setup_queues() {
  tuco::QueueData indices(*phys_device);
  transfer_family = indices.transferFamily.value();

  transfer_queue = device->get().getQueue(transfer_family, 0);
}

void StackBuffer::create_inter_buffer(vk::DeviceSize buffer_size,
                                      vk::MemoryPropertyFlags memory_properties,
                                      vk::Buffer &buffer,
                                      vk::DeviceMemory &memory) {
  auto create_info =
      vk::BufferCreateInfo({}, buffer_size,
                           vk::BufferUsageFlagBits::eTransferSrc |
                               vk::BufferUsageFlagBits::eTransferDst,
                           vk::SharingMode::eExclusive, 1, &transfer_family);

  buffer = device->get().createBuffer(create_info);

  auto mem_req = device->get().getBufferMemoryRequirements(buffer);

  auto mem_info = vk::MemoryAllocateInfo(
      mem_req.size,
      findMemoryType(*phys_device, mem_req.memoryTypeBits, memory_properties));

  memory = device->get().allocateMemory(mem_info);

  device->get().bindBufferMemory(buffer, memory, 0);
}

///
/// This function will sort the stack buffer so that data is compacted and
/// memory efficient, in terms of complexity this functions runs at O(n) but the
/// actual constant time moves are going to be a somewhat
/// expensive process so it would recommended to use during downtime in the
/// application (perhaps tracking CPU/GPU usage?)\
///
void StackBuffer::sort() {
  // complexity: we must touch each element at least once to check if it needs
  // to be moved
  //             after which we must perform a fairly expensive move operation
  //             thats dependent on the size of the allocation rather than the
  //             number of allocations

  // sort through allocations

  // check if ((next allocation offset) - (allocation offset + allocation size)
  // > MINIMUM_DISTANCE) MINIMUM_DISTANCE will check whether the amount of space
  // between the two allocations is large enough to be worth doing this
  // expensive operation in the first place

  // if the above statement passes then we must transfer our data from the stack
  // buffer into an intermediary buffer, we can accomplish this with
  // vkCmdCopyBuffer() command. once this data is in the buffer we can make a
  // second vkCmdCopyBuffer() call that will push our data back into the main
  // buffer at a new offset

  auto transfer_buffer = tuco::begin_command_buffer(*device, command_pool);

  // copy to buffer

  while (true) {
    VkDeviceSize gap = 0;
    VkDeviceSize current_offset = 0;
    VkDeviceSize data_size = 0;
    bool fragmented = false;
    for (auto it = allocations.begin(); it != allocations.end(); it++) {
      VkDeviceSize space_between =
          (it->first + it->second) - (gap + it->second);
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

    // TODO: if buffer is not large enough, make new inter_buffer
    VkBufferCopy src_info{};
    src_info.srcOffset = current_offset;
    src_info.dstOffset = 0;
    src_info.size = data_size;

    vkCmdCopyBuffer(transfer_buffer, buffer, inter_buffer, 1, &src_info);

    // copy back to src buffer
    VkBufferCopy dst_info{};
    dst_info.srcOffset = 0;
    dst_info.dstOffset = gap;
    dst_info.size = data_size;

    vkCmdCopyBuffer(transfer_buffer, inter_buffer, buffer, 1, &dst_info);
  }

  if (vkEndCommandBuffer(transfer_buffer) != VK_SUCCESS) {
    throw std::runtime_error(
        "could not create succesfully end transfer buffer");
  };

  auto submit_info =
      vk::SubmitInfo(0, nullptr, nullptr, 1, &transfer_buffer, 0, nullptr);

  transfer_queue.submit(submit_info);
  transfer_queue.waitIdle();

  device->get().free(command_pool, transfer_buffer);
}

VkDeviceSize StackBuffer::allocate(VkDeviceSize allocation_size) {
  if (allocation_size > buffer_size) {
    // not sure how to handle this case right now
    // will most likely need to create new vertex buffer, copy all the old data,
    // and destroy the old buffer.

    printf("[ERROR] - COULD NOT ALLOCATE INTO BUFFER, OUT OF SPACE \n");
    return 0;
  }

  // TODO: proper check for whether enough space exists (sort if not enough)
  buffer_size -= allocation_size;
  VkDeviceSize push_to = offset;
  offset += allocation_size;
  allocations.push_back(std::make_pair(push_to, offset));

  return push_to;
}

// EFFECTS: maps a given chunk of data to this buffer and returns the memory
// location of where it was mapped
VkDeviceSize StackBuffer::map(VkDeviceSize data_size, void *data) {
  VkDeviceSize memory_loc = allocate(data_size);

  vk::Buffer temp_buffer;
  vk::DeviceMemory temp_memory;
  create_inter_buffer(data_size, vk::MemoryPropertyFlagBits::eHostVisible,
                      temp_buffer, temp_memory);

  auto p_data = device->get().mapMemory(temp_memory, 0, data_size);

  memcpy(p_data, data, data_size);

  // map memory to buffer
  copyBuffer(temp_buffer, buffer, memory_loc, data_size);

  device->get().unmapMemory(temp_memory);
  device->get().freeMemory(temp_memory);
  device->get().destroyBuffer(temp_buffer);

  return memory_loc;
}

void StackBuffer::copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer,
                             VkDeviceSize dst_offset, VkDeviceSize data_size) {

  auto transfer_buffer = tuco::begin_command_buffer(*device, command_pool);

  // transfer between buffers
  VkBufferCopy copyData{};
  copyData.srcOffset = 0;
  copyData.dstOffset = dst_offset;
  copyData.size = data_size;

  vkCmdCopyBuffer(transfer_buffer, src_buffer, dst_buffer, 1, &copyData);

  tuco::end_command_buffer(*device, transfer_queue, command_pool,
                           transfer_buffer);
}

void StackBuffer::free(VkDeviceSize delete_offset) {
  // the only thing the user would have at this point is the offset
  // this is fine for a free call, we won't actually delete the data, but by
  // deleting the pointer to that data, when we sort the data later this "freed"
  // data will be overwritten and disappear
  VkDeviceSize current_offset = 0;
  for (auto it = allocations.begin(); it != allocations.end(); it++) {
    if (delete_offset == it->first) {
      // delete at location
      allocations.erase(it);
      break;
    }
  }
}

Pool::Pool(v::Device &device, std::vector<PoolCreateInfo> info) {
  api_device = &device;
  poolInfo = info;
  createPool();
}

void Pool::destroyPool() {
  for (size_t i = 0; i < pools.size(); i++) {
    api_device->get().destroyDescriptorPool(pools[i]);
  }
}

void Pool::createPool() {
  // create pool described by pool_info
  std::vector<VkDescriptorPoolSize> poolSizes;
  for (const auto &size : poolInfo) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = size.set_type;
    poolSize.descriptorCount = size.pool_size;
    poolSizes.push_back(poolSize);
  }

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets = POOL_MAX_SET_COUNT;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();

  size_t current_size = pools.size();
  pools.resize(current_size + 1);
  pools[current_size] = api_device->get().createDescriptorPool(poolInfo);
}

VkDescriptorPool &Pool::getCurrentPool() { return pools[pools.size() - 1]; }

VkResult Pool::attemptAllocation(v::Device &device, uint32_t setCount,
                                 VkDescriptorSetLayout layout,
                                 VkDescriptorSet *sets) {
  std::vector<VkDescriptorSetLayout> layouts(setCount, layout);

  VkDescriptorSetAllocateInfo allocateInfo{};
  VkDescriptorPool pool = getCurrentPool();

  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = pool;
  allocateInfo.descriptorSetCount = setCount;
  allocateInfo.pSetLayouts = layouts.data();

  return vkAllocateDescriptorSets(device.get(), &allocateInfo, sets);
}

void Pool::allocateDescriptorSets(v::Device &device, uint32_t setCount,
                                  VkDescriptorSetLayout layout,
                                  VkDescriptorSet *sets) {

  VkResult result = attemptAllocation(device, setCount, layout, sets);
  if (result == VK_SUCCESS) {
    return;
  }
  if (result != VK_ERROR_OUT_OF_POOL_MEMORY) {
    ERR("Failed to allocate memory for descriptor sets.");
    throw std::runtime_error("");
  }

  // queue pool for deallocation..
  // TODO...

  // attempt to make second allocation.
  createPool();
  result = attemptAllocation(device, setCount, layout, sets);
  if (result != VK_SUCCESS) {
    ERR("Failed to allocate memory for descriptor sets on second "
        "attempt");
    throw std::runtime_error("");
  }
}

size_t Pool::allocate(VkDeviceSize allocation_size) {
  // decprecated, use allocateDescriptorSets().
  return pools.size() - 1;
}

uint32_t mem::findMemoryType(v::PhysicalDevice &physical_device,
                             uint32_t typeFilter,
                             vk::MemoryPropertyFlags properties) {
  auto mem_prop = physical_device.get().getMemoryProperties();

  // uint32_t suitableMemoryForBuffer = 0;
  for (uint32_t i = 0; i < mem_prop.memoryTypeCount; i++) {
    if (typeFilter & (1 << i) &&
        ((mem_prop.memoryTypes[i].propertyFlags & properties) == properties)) {
      return i;
    }
  }

  throw std::runtime_error("could not find appropriate memory type");
}
