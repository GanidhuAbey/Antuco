#pragma once

#include <vulkan/vulkan.hpp>

#include <map>
#include <vector>

#include "vulkan_wrapper/instance.hpp"

namespace v {
class PhysicalDevice {
private:
    vk::PhysicalDevice physical_device;
    std::shared_ptr<v::Instance> m_instance;


    void pick_physical_device(Instance* instance);
    uint32_t score_physical_device(vk::PhysicalDevice);

    void set_device_limits();

public:
    PhysicalDevice() {}

    PhysicalDevice(std::shared_ptr<v::Instance> instance);
    ~PhysicalDevice();

    vk::PhysicalDevice& get() { return physical_device; }

    operator vk::PhysicalDevice() { return physical_device; }
    operator VkPhysicalDevice() { return physical_device; }
};
}
