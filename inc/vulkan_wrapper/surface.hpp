#pragma once

#include <vulkan/vulkan.hpp>

#include "vulkan_wrapper/instance.hpp"

namespace v {
class Surface {
private:
    vk::SurfaceKHR surface;
    void create_vulkan_surface(Instance instance, GLFWwindow* window);
public:
    Surface(Instance instance, GLFWwindow* window) { create_vulkan_surface(instance, window); }
    ~Surface() {}

    operator VkSurfaceKHR() { return surface; }
    
};
}
