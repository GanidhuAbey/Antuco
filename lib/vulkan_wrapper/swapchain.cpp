#include "vulkan_wrapper/swapchain.hpp"

#include "logger/interface.hpp"
#include "queue.hpp"

using namespace v;

Swapchain::Swapchain(std::shared_ptr<v::PhysicalDevice> p_physical_device, std::shared_ptr<Device> device, Surface *p_surface) {
    init(p_physical_device, device, p_surface);
}

void Swapchain::init(std::shared_ptr<v::PhysicalDevice> p_physical_device, std::shared_ptr<Device> device, v::Surface* surface) {
    Swapchain::device = device;
    create_swapchain(p_physical_device, device, surface);
}

Swapchain::~Swapchain() {
    destroy();
}

void Swapchain::destroy() {
    for (auto& image : swapchain_images) {
        image.destroy_image_view();
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
    std::shared_ptr<v::PhysicalDevice> p_physical_device,
    std::shared_ptr<Device> device, 
    Surface *p_surface) {
    //query the surface capabilities
    auto capabilities = p_physical_device->get().getSurfaceCapabilitiesKHR(
        p_surface->get()
    );

    auto format_count = uint32_t(0);
    auto result = p_physical_device->get().getSurfaceFormatsKHR(
        p_surface->get(), 
        &format_count, 
        nullptr
    );

    ASSERT(result == vk::Result::eSuccess, "could not get swapchain format count");

    auto formats = std::vector<vk::SurfaceFormatKHR>(format_count);
    result = p_physical_device->get().getSurfaceFormatsKHR(
        p_surface->get(), 
        &format_count, 
        formats.data()
    );

    ASSERT(result == vk::Result::eSuccess, "could not retrieve swapchain");

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
        p_surface->get(), 
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
    br::ImageViewCreateInfo image_info{};
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

    br::ImageData data;
    data.name = "swapchain";
    data.image_view_info = image_info;

    //transfer swapchain to present layout
    for (size_t i = 0; i < swapchain_images.size(); i++) {
        swapchain_images[i].init(
            p_physical_device,
            device, 
            images[i], 
            data,
            true);

        swapchain_images[i].transfer(
            vk::ImageLayout::ePresentSrcKHR, 
            device->get_graphics_queue()
        );
    }
}
