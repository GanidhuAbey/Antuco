#pragma once

#include <vulkan/vulkan.hpp>

#include "physical_device.hpp"
#include "surface.hpp"
#include "config.hpp"

namespace v {
class Device {
private:        
    vk::Device device;
    uint32_t graphics_family;
    uint32_t present_family;
    uint32_t transfer_family;

    vk::Queue graphics_queue;
    vk::Queue present_queue;
    vk::Queue transfer_queue;

    std::shared_ptr<v::PhysicalDevice> m_phys_device;
    std::shared_ptr<v::Surface> m_surface;

    std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
public:
    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device() {}
    //print debug enables debug printing in shaders, not supported by all GPU's
    Device(std::shared_ptr<v::PhysicalDevice> phys_device, std::shared_ptr<v::Surface> surface, bool print_debug);
    ~Device();

    vk::Device get() { return device; }

    uint32_t& get_graphics_family() { return graphics_family; }
    uint32_t& get_present_family() { return present_family; }
    uint32_t& get_transfer_family() { return transfer_family; }

    vk::Queue& get_graphics_queue() { return graphics_queue; }
    vk::Queue& get_present_queue() { return present_queue; }
    vk::Queue& get_transfer_queue() { return transfer_queue; }

private:
    void create_logical_device(PhysicalDevice* physical_device, Surface* surface, bool print_debug);
    bool check_device_extensions(PhysicalDevice* phys_device, std::vector<const char*> extensions, uint32_t extensions_count);
};
}
