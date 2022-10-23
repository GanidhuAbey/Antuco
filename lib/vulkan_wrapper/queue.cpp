#include "vulkan_wrapper/queue.hpp"

#include "vulkan_wrapper/device.hpp"


using namespace v;

Queue::Queue(PhysicalDevice& device, Surface& surface) {
    find_queue_families(device, surface); 
}

void Queue::set_required(std::vector<QueueTypes> queue_types) {
    for (const auto& type : queue_types) {
        switch (type) {
            case Render:
                need_render = true;
                continue;
            case Compute:
                need_compute = true;
                continue;
            case Present:
                need_present = true;
                continue;
            case Transfer:
                need_transfer = true;
                continue;
        } 
    } 
}

bool Queue::found_queues() {
    return (!need_present || present_family.has_value()) &&
            (!need_compute || compute_family.has_value()) &&
            (!need_transfer || transfer_family.has_value()) &&
            (!need_render || render_family.has_value());
}

void Queue::find_queue_families(v::PhysicalDevice& device, v::Surface& surface) {
    //logic to fill indices struct up
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        device.get(), 
        &queueCount, 
        nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
    
    vkGetPhysicalDeviceQueueFamilyProperties(
        device.get(), 
        &queueCount, 
        queueFamilies.data());

    int i = 0;

    for (const auto& queue : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            device.get(), 
            i, 
            surface.get(), 
            &presentSupport);

        if (presentSupport) {
            present_family = i;
        }
        if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            render_family = i;
        }
        if (queue.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            transfer_family = i;
        }
        if (queue.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            compute_family = i;
        }
        if (found_queues()) {
            break;
        }
        i++;
    }

    if (found_queues()) {
        printf(
        "[ERROR] - findQueueData : could not find desired queues \n");
        throw std::runtime_error("");
    }
}


