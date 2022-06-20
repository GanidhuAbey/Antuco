#pragma once

#include <vulkan/vulkan.hpp>

#include "vulkan_wrapper/instance.hpp"

namespace v {
class Surface {
private:
    vk::Instance instance;
    vk::SurfaceKHR surface;
    void create_vulkan_surface(Instance& instance, GLFWwindow* window);
public:
    Surface(Instance& instance, GLFWwindow* window) {
        Surface::instance = instance.get();
        create_vulkan_surface(instance, window); 
    }
    ~Surface() {
        instance.destroySurfaceKHR(surface);
    }

    operator VkSurfaceKHR() { return surface; }
    
};
}
