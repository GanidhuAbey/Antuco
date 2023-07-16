#include "descriptor_set.hpp"

using namespace tuco;

void ResourceCollection::init(
const v::Device& device, VkDescriptorType type, 
VkDescriptorSetLayout layout, uint32_t resource_size, mem::Pool& pool) {
    ResourceCollection::device = const_cast<v::Device*>(&device);

    sets.resize(resource_size);
    descriptor_type = type;

    create_set(layout, pool);
}

bool ResourceCollection::check_size(size_t i) {
    if (i >= sets.size()) {
        LOG("[ERROR] - invsalid allocation");
        throw std::runtime_error("exection forcefully stopped");
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

void ResourceCollection::destroy() {
}

void ResourceCollection::create_set(VkDescriptorSetLayout layout, 
mem::Pool& pool) {
    std::vector<VkDescriptorSetLayout> layouts(sets.size(), layout);

    pool_index = pool.allocate(sets.size());

    VkDescriptorSetAllocateInfo allocateInfo{}; 
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = pool.pools[pool_index];
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(sets.size());
    allocateInfo.pSetLayouts = layouts.data();

    VkResult result = vkAllocateDescriptorSets(
        device->get(), 
        &allocateInfo, 
        sets.data());

    if (result != VK_SUCCESS) {
        msg::print_line("[ERROR CODE " + std::to_string(result) + "] - could not allocate sets");
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

    size_t image = 0;
    
    if (image_info.size() > 1) {
        image = i;
    }

    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstBinding = binding;
    writeInfo.dstSet = sets[i];
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = descriptor_type;
    if (image_add) {
        writeInfo.pImageInfo = &image_info[image];
    }
    if (buffer_add) {
        writeInfo.pBufferInfo = &buffer_info;
    }
    writeInfo.dstArrayElement = 0;

    vkUpdateDescriptorSets(device->get(), 1, &writeInfo, 0, nullptr);
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
void ResourceCollection::add_image(VkImageLayout image_layout, mem::Image& image, vk::Sampler& image_sampler) {
	if (buffer_add) {
		LOG("[ERROR] - attempted to add image data to buffer set");
		return;
	}

    image_add = true;

    image_info.resize(1);
    image_info[0].imageLayout = image_layout;
    image_info[0].imageView = image.get_api_image_view();
    image_info[0].sampler = image_sampler;
}

void ResourceCollection::add_image(VkImageLayout image_layout,
        std::vector<mem::Image>& images, 
        vk::Sampler& image_sampler) {

	if (buffer_add) {
		LOG("[ERROR] - attempted to add image data to buffer set");
		return;
	}

    if (images.size() != sets.size()) {
        msg::print_line("[ERROR] - image count must equal 1 or resource size");
        return;
    }

    image_add = true;

    for (mem::Image& image : images) {
        VkDescriptorImageInfo info{};

        info.imageLayout = image_layout;
        info.imageView = image.get_api_image_view();
        info.sampler = image_sampler;

        image_info.push_back(info);
    }
}

void ResourceCollection::set_binding(uint32_t binding) {
	ResourceCollection::binding = binding;
}
