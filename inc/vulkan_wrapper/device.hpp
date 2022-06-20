#pragma once

#include <vulkan/vulkan.hpp>

#include "vulkan_wrapper/physical_device.hpp"

namespace v {
class Device {
public:
    //print debug enables debug printing in shaders, not supported by all GPU's
    Device(PhysicalDevice phys_device, bool print_debug);
    ~Device();

private:
    vk::Device device;
    void create_logical_device(PhysicalDevice physical_device, bool print_debug);

};
}
