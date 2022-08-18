#include "vulkan_wrapper/swapchain.hpp"

#include "logger/interface.hpp"
#include "queue.hpp"

using namespace v;

Swapchain::Swapchain(
PhysicalDevice& physical_device, 
Device& device, 
Surface& surface) {
  Swapchain::device = &device;

  init(physical_device, surface);
}

void Swapchain::init(v::PhysicalDevice& physical_device, v::Surface& surface) { 
  create_swapchain(physical_device, device, surface);
}

Swapchain::~Swapchain() {
  destroy();
}

void Swapchain::destroy() {
    for (auto& image : swapchain_images) {
<<<<<<< HEAD
        image.destroy_image_view();
=======
      image.destroy();
>>>>>>> cedc8eaf0dabf92dae9736e7806a58989fd506a8
    }
    
    device->get().destroySwapchainKHR(swapchain);
}

vk::SurfaceFormatKHR Swapchain::choose_best_surface_format(
    std::vector<vk::SurfaceFormatKHR> formats) {
    for (const auto& format : formats) {
      if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && 
          format.format == vk::Format::eR8G8B8A8Srgb) {
        return format;
      } 
    }

    return formats[0];
}

void Swapchain::create_swapchain(
    PhysicalDevice& physical_device, 
    Device* device, 
    Surface& surface) {
    //query the surface capabilities
    auto capabilities = physical_device.get().getSurfaceCapabilitiesKHR(
        surface.get()
    );

    auto format_count = uint32_t(0);
    auto result = physical_device.get().getSurfaceFormatsKHR(
        surface.get(), 
        &format_count, 
        nullptr
    );

    if (result != vk::Result::eSuccess) {
      msg::print_line("[ERROR] - create_swapchain : could not get format count");
      throw std::runtime_error("");
    }

    auto formats = std::vector<vk::SurfaceFormatKHR>(format_count);
    result = physical_device.get().getSurfaceFormatsKHR(
        surface.get(), 
        &format_count, 
        formats.data()
    );
    if (result != vk::Result::eSuccess) {
      msg::print_line("[ERROR] - create_swapchain : could not get formats");
      throw std::runtime_error("");
    }

    auto surface_format = choose_best_surface_format(formats);
    auto image_num = capabilities.minImageCount;

    if (capabilities.maxImageCount > 0 && 
        image_num > capabilities.maxImageCount) {
      image_num = capabilities.maxImageCount;
    }

    format = surface_format.format;
    extent = capabilities.currentExtent;
    auto swap_info = vk::SwapchainCreateInfoKHR(
        {}, 
        surface.get(), 
        image_num, 
        format, 
        surface_format.colorSpace, 
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment | 
        vk::ImageUsageFlagBits::eTransferDst,
        vk::SharingMode::eExclusive,
        1,
        &device->get_graphics_family(),
        capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::PresentModeKHR::eImmediate, //TODO: add check for support
        true,
        {});

    swapchain = device->get().createSwapchainKHR(swap_info);

    //grab swapchain images
    mem::ImageViewCreateInfo image_info{};
    image_info.aspect_mask = vk::ImageAspectFlagBits::eColor;
    image_info.format = format;
 
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(
        device->get(), 
        swapchain, 
        &swapchain_image_count, 
        nullptr
    );

    std::vector<vk::Image> images;
    images = device->get().getSwapchainImagesKHR(swapchain);
    swapchain_images.resize(images.size());

    mem::ImageData data;
    data.name = "swapchain";
    data.image_view_info = image_info;

    //transfer swapchain to present layout
    for (size_t i = 0; i < swapchain_images.size(); i++) {
        swapchain_images[i].init(
            physical_device, 
            *device, 
            images[i], 
            data);

        swapchain_images[i].transfer(
            vk::ImageLayout::ePresentSrcKHR, 
            device->get_graphics_queue()
        );
    }
}
