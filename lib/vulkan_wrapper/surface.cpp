#include "vulkan_wrapper/surface.hpp"

#include <GLFW/glfw3.h>

using namespace v;

void Surface::create_vulkan_surface(Instance instance, GLFWwindow* window) {
    VkSurfaceKHR surface_tmp;
    glfwCreateWindowSurface(instance, window, nullptr, &surface_tmp);

    surface = vk::SurfaceKHR(surface_tmp);
}

