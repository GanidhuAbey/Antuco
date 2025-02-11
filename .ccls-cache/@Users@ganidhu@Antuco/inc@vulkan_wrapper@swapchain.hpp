#pragma once

#include <vulkan/vulkan.hpp>

#include "physical_device.hpp"
#include "device.hpp"
#include "surface.hpp"

#include "memory_allocator.hpp"

namespace v {
  class Swapchain {
    private:
      v::Device* device;

      vk::Format format;
      vk::Extent2D extent;

      vk::SwapchainKHR swapchain;
      std::vector<mem::Image> swapchain_images;

    public:
      Swapchain(
          PhysicalDevice& physical_device, 
          Device& device, 
          Surface& surface);

      ~Swapchain();

      vk::SwapchainKHR& get() { return swapchain; }

      mem::Image& get_image(int i) { 
        if (i > swapchain_images.size()) {
          throw std::runtime_error("invalid index location");
        }
        return swapchain_images[i];
      }

      void init(v::PhysicalDevice& physical_device, v::Surface& surface);
      void destroy();

      vk::Format get_format() { return format; }
      vk::Extent2D get_extent() { return extent; }
      
      uint32_t get_image_size() { 
        return static_cast<uint32_t>(swapchain_images.size()); 
      }

    private:
      void create_swapchain(
          PhysicalDevice& physical_device, 
          Device* device, 
          Surface& surface);

      vk::SurfaceFormatKHR choose_best_surface_format(
          std::vector<vk::SurfaceFormatKHR> formats);
  };
}
