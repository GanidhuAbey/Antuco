#include "descriptor_set.hpp"

using namespace tuco;

void ResourceCollection::init(VkDevice& device, VkDescriptorType type, VkDescriptorSetLayout layout, size_t resource_size, mem::Pool& pool) {
    p_device = device;

    sets.resize(resource_size);
    descriptor_type = type;

    //create_set(resource_size, layout, pool);
}

bool ResourceCollection::check_size(size_t i) {
    if (i >= sets.size()) {
        LOG("[ERROR] - invalid allocation");

        return false;
    }

    return true;
}

VkDescriptorSet ResourceCollection::get_api_set(size_t i) {
    if (!check_size(i)) {
        return nullptr;
    }

    return sets[i];
}

void ResourceCollection::create_set(uint32_t size, VkDescriptorSetLayout layout, mem::Pool& pool) {
    std::vector<VkDescriptorSetLayout> layouts(size, layout);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = pool.pools[pool.allocate(p_device, size)];
    allocateInfo.descriptorSetCount = size;
    allocateInfo.pSetLayouts = layouts.data();

    VkResult result = vkAllocateDescriptorSets(p_device, &allocateInfo, sets.data());

    if (result != VK_SUCCESS) {
        LOG("[ERROR CODE " + std::to_string(result) + "] - could not allocate sets");
        return;
    }
}

void ResourceCollection::update_set() {
    for (size_t i = 0; i < sets.size(); i++) {
        update_set(i);
    }
}

void ResourceCollection::update_set(size_t i) {
    if (!check_size(i)) {
        return;
    }

    if (!buffer_add && !image_add) {
        LOG("[ERROR] - no resource supplied to resource collection");
    }

    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstBinding = binding;
    writeInfo.dstSet = sets[i];
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = descriptor_type;
    if (image_add) {
        writeInfo.pImageInfo = &image_info;
    }
    if (buffer_add) {
        writeInfo.pBufferInfo = &buffer_info;
    }
    writeInfo.dstArrayElement = 0;

    vkUpdateDescriptorSets(p_device, 1, &writeInfo, 0, nullptr);
}

void ResourceCollection::remove_buffer() {
    buffer_add = false;
}

void ResourceCollection::remove_image() {
    image_add = false;
}

void ResourceCollection::add_buffer(VkBuffer buffer, VkDeviceSize buffer_offset, VkDeviceSize buffer_range) {
	if (image_add) {
		LOG("[ERROR] - attempted to add buffer data to image set");
		return;
	}

    buffer_add = true;

    buffer_info.buffer = buffer;
    buffer_info.offset = buffer_offset;
    buffer_info.range = buffer_range;
}
void ResourceCollection::add_image(VkImageLayout image_layout, VkImageView& image_view, VkSampler& image_sampler) {
	if (buffer_add) {
		LOG("[ERROR] - attempted to add image data to buffer set");
		return;
	}

    image_add = true;

    image_info.imageLayout = image_layout;
    image_info.imageView = image_view;
    image_info.sampler = image_sampler;
}

void ResourceCollection::set_binding(uint32_t binding) {
	ResourceCollection::binding = binding;
}