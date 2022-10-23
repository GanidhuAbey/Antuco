#include "vulkan_wrapper/device.hpp"

#include "queue_data.hpp"

#include <vector>
#include <set>

using namespace v;

Device::Device(PhysicalDevice& phys_device, Surface& surface, bool print_debug) {
    create_logical_device(phys_device, surface, print_debug); 
}
Device::~Device() {
    device.destroy();
}

bool Device::check_device_extensions(PhysicalDevice& phys_device, std::vector<const char*> extensions, uint32_t extensions_count) {
	uint32_t all_extensions_count;
	vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &all_extensions_count, nullptr);
	std::vector<VkExtensionProperties> all_extensions(all_extensions_count);
	vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &all_extensions_count, all_extensions.data());

	//check if required extensions are supported
	for (uint32_t i = 0; i < extensions_count; i++) {
		bool extension_found = false;
		for (uint32_t j = 0; j < all_extensions_count; j++) {
			if (strcmp(extensions[i], all_extensions[j].extensionName)) {
				extension_found = true;
			}
		}
		if (!extension_found) {
			return false;
		}
	}

	return true;

}

void Device::create_logical_device(PhysicalDevice& physical_device, Surface& surface, bool print_debug) {
#ifdef NDEBUG 
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
	//first we need to retrieve queue data from the computer
	auto indices = tuco::QueueData(physical_device, surface);

	graphics_family = indices.graphicsFamily.value();
	present_family = indices.presentFamily.value();
	transfer_family = indices.transferFamily.value();

    auto queue_count = uint32_t();
    auto queue_data = std::vector<vk::DeviceQueueCreateInfo>();

	//this way, if the graphics family and the present family are the same, then the set will only keep one value.
	std::set<uint32_t> queue_indices = {graphics_family, present_family};
	const float queue_priority = 1.0;

	for (const auto& i : queue_indices) {
        auto info = vk::DeviceQueueCreateInfo(
                {},
                i,
                1,
                &queue_priority
                );
		
		queue_count++;
		queue_data.push_back(info);
	}

#ifdef APPLE_M1
    device_extensions.push_back("VK_KHR_portability_subset");	
#endif

    if (enableValidationLayers && print_debug) {
	    device_extensions.push_back("VK_KHR_shader_non_semantic_info"); //shader print debug extension
    }

    //check if device extension needed is supported
	if (!check_device_extensions(physical_device, device_extensions, device_extensions.size())) {
		printf("[ERROR] - create_logical_device: could not find support for neccessary device extensions");
		throw std::runtime_error("");
	}

    vk::PhysicalDeviceFeatures device_features;
    auto device_info = vk::DeviceCreateInfo(
            {}, 
            queue_count, 
            queue_data.data(), 
            0, 
            nullptr, 
            static_cast<uint32_t>(device_extensions.size()), 
            device_extensions.data(),
            &device_features
        );

    device = physical_device.get().createDevice(device_info);

    graphics_queue = device.getQueue(graphics_family, 0);
    present_queue = device.getQueue(present_family, 0);
    transfer_queue = device.getQueue(transfer_family, 0);
}
