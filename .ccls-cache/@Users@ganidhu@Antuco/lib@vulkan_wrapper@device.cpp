/*
#include "vulkan_wrapper/device.hpp"

#include "queue.hpp"

#include <vector>

using namespace v;

Device::Device(PhysicalDevice phys_device, bool print_debug) {
    create_logical_device(phys_device, print_debug); 
}
Device::~Device() {}

void Device::create_logical_device(PhysicalDevice physical_device, bool print_debug) {
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
	    device_extensions.push_back("VK_KHR_shader_non_semantic_info"); //shader print debug extension
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
*/
