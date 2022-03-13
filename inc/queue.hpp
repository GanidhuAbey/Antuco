#pragma once

#include "logger/interface.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>
#include <iostream>

namespace tuco {
class QueueData {
public:
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

public:
    QueueData(VkPhysicalDevice device, VkSurfaceKHR surface) {
        find_queue_families(device, surface);
    }
    QueueData(VkPhysicalDevice device) {
        find_queues(device);
    }
public:
    bool is_complete() { return graphicsFamily.has_value() && presentFamily.has_value(); }

private:
    void find_queues(VkPhysicalDevice device) {
        //logic to fill indices struct up
        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamilies.data());

        int i = 0;

        bool found = false;
        for (const auto& queue : queueFamilies) {
            if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
            }
            if (queue.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                transferFamily = i;
            }
            if (transferFamily.has_value() && graphicsFamily.has_value()) {
                found = true;
                break;
            }
            i++;
        }

        if (!found) {
            LOG("required queues not found");
        }
    }
    void find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
        //logic to fill indices struct up
        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamilies.data());

        //NOTE: the reason for the const auto& is to specify that the data being read from the for loop is NOT being modified
        int i = 0;

        for (const auto& queue : queueFamilies) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                presentFamily = i;
            }
            if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
            }
            if (queue.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                transferFamily = i;
            }
            if (is_complete()) {
                break;
            }
            i++;
        }

        if (!is_complete()) {
            printf("[ERROR] - findQueueData : could not find desired queues \n");
            throw std::runtime_error("");
        }
    }
};
}
