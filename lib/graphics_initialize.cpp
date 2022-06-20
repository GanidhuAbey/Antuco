#include "api_graphics.hpp"
#include "logger/interface.hpp"
#include "queue.hpp"

#include <stdint.h>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <map>
#include <set>

using namespace tuco;


void GraphicsImpl::destroy_initialize() {
#ifdef NDEBUG 
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
	vkDestroyDevice(*p_device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
}


void GraphicsImpl::create_logical_device() {

#ifdef NDEBUG 
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
	//first we need to retrieve queue data from the computer
	QueueData indices(physical_device, surface);

	graphics_family = indices.graphicsFamily.value();
	present_family = indices.presentFamily.value();

	//create logical device
	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = nullptr;
	deviceInfo.flags = 0;

	//this way, if the graphics family and the present family are the same, then the set will only keep one value.
	std::set<uint32_t> queue_indices = {graphics_family, present_family};

	std::vector<VkDeviceQueueCreateInfo> queue_data;
	const float queue_priority = 1.0;
	for (const auto& i : queue_indices) {
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pNext = nullptr;
		queueInfo.flags = 0;
		queueInfo.queueFamilyIndex = i;
		queueInfo.queueCount = 1; // i feel like we could do more by creating the maximum amount of queues...
		queueInfo.pQueuePriorities = &queue_priority; //setting this to 1 in both is essentially the same as letting the driver handle
													  //how to best organize the queues.
		
		deviceInfo.queueCreateInfoCount++;
		queue_data.push_back(queueInfo);
	}

	deviceInfo.pQueueCreateInfos = queue_data.data();

#ifdef APPLE_M1
    device_extensions.push_back("VK_KHR_portability_subset");	
#endif
    if (enableValidationLayers && print_debug) {
	    device_extensions.push_back("VK_KHR_shader_non_semantic_info");
    }

    //enable raytracing support (RT)
	if (raytracing) {
		//enable acceleration structure extensions
		device_extensions.push_back("VK_KHR_acceleration_structure");

		device_extensions.push_back("VK_KHR_deferred_host_operations");
		device_extensions.push_back("VK_KHR_buffer_device_address");
		device_extensions.push_back("VK_KHR_maintenance3");
		device_extensions.push_back("VK_EXT_descriptor_indexing");

		//enable ray tracing pipeline extensions
		device_extensions.push_back("VK_KHR_ray_tracing_pipeline");

		device_extensions.push_back("VK_KHR_spirv_1_4");
	}

    //check if device extension needed is supported
	if (!check_device_extensions(device_extensions, device_extensions.size())) {
		printf("[ERROR] - create_logical_device: could not find support for neccessary device extensions");
		throw std::runtime_error("");
	}

	deviceInfo.enabledExtensionCount = device_extensions.size();
	deviceInfo.ppEnabledExtensionNames = device_extensions.data();

	//i don't really care about adding any specific features right now...
	VkPhysicalDeviceFeatures device_features{};
	deviceInfo.pEnabledFeatures = &device_features;

    VkDevice device;
	VkResult device_result = vkCreateDevice(
            physical_device.get(), 
            &deviceInfo,
            nullptr,
            &device);
    p_device = std::make_shared<VkDevice>(device);
	if (device_result != VK_SUCCESS) {
		printf("[ERROR %d] - create_logical_device: could not create vulkan device object", device_result);
		throw std::runtime_error("");
	}

	vkGetDeviceQueue(device, graphics_family, 0, &graphics_queue);
	vkGetDeviceQueue(device, present_family, 0, &present_queue);
}

//checks for and enables required raytracing extensions
//crashes if raytracing is enabled on unsupported GPU.
void GraphicsImpl::enable_raytracing() {

}

void GraphicsImpl::create_command_pool() {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.flags = 0;
	poolInfo.queueFamilyIndex = graphics_family; //these command buffers we are allocating are for drawing

	//create the command pool
	if (vkCreateCommandPool(*p_device, &poolInfo, nullptr, &command_pool) != VK_SUCCESS) {
		printf("[ERROR] - create_command_pool : could not create command pool");
		throw std::runtime_error("");
	}
}

bool GraphicsImpl::check_device_extensions(std::vector<const char*> extensions, uint32_t extensions_count) {
	uint32_t all_extensions_count;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &all_extensions_count, nullptr);
	std::vector<VkExtensionProperties> all_extensions(all_extensions_count);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &all_extensions_count, all_extensions.data());

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

