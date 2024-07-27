#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hpp"
#include "physical_device.hpp"
#include "surface.hpp"

#include "memory_allocator.hpp"
#include <bedrock/image.hpp>

namespace v {
class Swapchain {
private:
  std::shared_ptr<v::Device> device;

  vk::Format format;
  vk::Extent2D extent;

  vk::SwapchainKHR swapchain;
  std::vector<br::Image> swapchain_images;

public:
  Swapchain(std::shared_ptr<v::PhysicalDevice> p_physical_device, std::shared_ptr<Device> device, Surface *p_surface);
  Swapchain() = default;

  ~Swapchain();

  vk::SwapchainKHR &get() { return swapchain; }

  br::Image &get_image(int i) {
    if (i > swapchain_images.size()) {
      throw std::runtime_error("invalid index location");
    }
    return swapchain_images[i];
  }

  void init(std::shared_ptr<v::PhysicalDevice> p_physical_device, std::shared_ptr<Device> device, v::Surface *p_surface);
  void destroy();

  vk::Format get_format() { return format; }
  vk::Extent2D get_extent() { return extent; }

  uint32_t getSwapchainSize() {
    return static_cast<uint32_t>(swapchain_images.size());
  }

private:
  void create_swapchain(std::shared_ptr<v::PhysicalDevice> p_physical_device, std::shared_ptr<Device> device,
                        Surface *p_surface);

  vk::SurfaceFormatKHR
  choose_best_surface_format(std::vector<vk::SurfaceFormatKHR> formats);
};
} // namespace v
