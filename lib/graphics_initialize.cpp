#include "api_graphics.hpp"
#include "queue.hpp"

#include <stdint.h>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <map>
#include <set>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

using namespace tuco;

//debug callback function
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	//arrow is used in pointers of objects/structs
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void GraphicsImpl::destroy_initialize() {
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	
	if (enableValidationLayers) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr) {
			//why dont I need messengar?
			func(instance, debug_messenger, nullptr);
		}
		else {
			printf("[ERROR] - destroy_initialize : could not destroy debug_messenger \n");
			throw std::runtime_error("");
		}
	}

	vkDestroyInstance(instance, nullptr);
}

void GraphicsImpl::create_instance(const char* appName) {
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = appName;
	appInfo.applicationVersion = API_VERSION_1_0;
	appInfo.pEngineName = "Antuco";
	appInfo.engineVersion = API_VERSION_1_0;
	appInfo.apiVersion = VK_API_VERSION_1_2;
	//create debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = &debug_callback;

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;

	//determine layer count (we only really care about the debug validation layers)
	if (enableValidationLayers && validation_layer_supported(validation_layers)) {
		printf("[DEBUG] - VALIDATION LAYERS ENABLED \n");
		
		instanceInfo.enabledLayerCount = validation_layers.size();
		instanceInfo.ppEnabledLayerNames = validation_layers.data();

		instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo; //TODO: extend debug structure
	}
	else {
		if (enableValidationLayers) printf("[WARNING] - could not enable validation layers \n");

		instanceInfo.enabledLayerCount = 0;
		instanceInfo.ppEnabledLayerNames = nullptr;

		instanceInfo.pNext = nullptr;
	}

	//determine extensions
	uint32_t glfw_extension_count;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#ifdef APPLE_M1
	extensions.push_back("VK_KHR_get_physical_device_properties2");
#else
	if (raytracing) {
		extensions.push_back("VK_KHR_get_physical_device_properties2");
	}
#endif
	
	if (check_extensions_supported(extensions.data(), static_cast<uint32_t>(extensions.size()))) {
		instanceInfo.enabledExtensionCount = extensions.size();
		instanceInfo.ppEnabledExtensionNames = extensions.data();
	}
	else {
		printf("[ERROR] - required instance extensions not supported \n");
		throw std::runtime_error("");
	}

  VkResult instance_response = vkCreateInstance(&instanceInfo, nullptr, &instance);
	if (instance_response != VK_SUCCESS) {
		printf("[ERROR CODE: %d] - create_instance() : could not create instance \n", instance_response);
		throw std::runtime_error("");
	}

	//create debug messengar here?
	VkResult debug_result = create_debug_utils_messengar_utils(instance, &debugInfo, nullptr, &debug_messenger);
	if (debug_result != VK_SUCCESS) {
		printf("[ERROR] - create_instance - could not create debug messenger, ERROR CODE: %u \n", debug_result);
	}
}

VkResult GraphicsImpl::create_debug_utils_messengar_utils(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger) {
	//basically casting the return of vkGetInstanceProcAddr to a pointer function of the function being loaded.
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	//check if function successfully loaded
	if (func != nullptr) {
		//why dont I need messengar?
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		//TODO: figure out best error message
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

}

void GraphicsImpl::pick_physical_device() {
	//query all physical devices we have
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
	std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
	vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());

	//score each device
	std::multimap<uint32_t, VkPhysicalDevice> device_score;
	for (const auto& current_device : physical_devices) {
		uint32_t score = score_physical_device(current_device);
		device_score.insert(std::make_pair(score, current_device));
	}

	//pick device with highest score
	uint32_t biggest = 0;
	VkPhysicalDevice current_best_device = nullptr;
	for (auto i = device_score.rbegin(); i != device_score.rend(); i++) {
		if (i->first > biggest) {
			biggest = i->first;
			current_best_device = i->second;
		}
	}
	if (current_best_device == nullptr) {
		printf("[ERROR] - could not find suitable GPU from this device");
		throw std::runtime_error("");
	}
	
	if (enableValidationLayers) {
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(current_best_device, &device_properties);
		
		printf("[DEBUG] - CHOSEN GPU: %s \n", device_properties.deviceName);
		printf("[DEBUG] - MAXIMUM PUSH CONSTANT RANGE: %u \n", device_properties.limits.maxPushConstantsSize);

	}

	//make the best physical device the one we'll use for the program
	physical_device = current_best_device;
}

uint32_t GraphicsImpl::score_physical_device(VkPhysicalDevice physical_device) {
	uint32_t score = 1; //if the device exist, its already better than nothing...

	//retrieve the features/properties of the physical device
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;

	vkGetPhysicalDeviceProperties(physical_device, &device_properties);
	vkGetPhysicalDeviceFeatures(physical_device, &device_features);

	//is the gpu discrete?
	if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	//can query other features/properties of the gpu and manipulate the score to get
	//the best possible gpu for our program
	//...

	return score;
}

void GraphicsImpl::create_logical_device() {
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


	VkResult device_result = vkCreateDevice(physical_device, &deviceInfo, nullptr, &device);
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
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &command_pool) != VK_SUCCESS) {
		printf("[ERROR] - create_command_pool : could not create command pool");
		throw std::runtime_error("");
	}

}

bool GraphicsImpl::validation_layer_supported(std::vector<const char*> names) {
	//query all the supported layers
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	std::vector<VkLayerProperties> layers(layer_count);
	if (layer_count == 0) return false;
	vkEnumerateInstanceLayerProperties(&layer_count, layers.data());

	//check if the layer we are looking for is supported
	bool layer_found = false;
	for (size_t i = 0; i < names.size(); i++) {
		layer_found = false;
		for (size_t j = 0; j < layers.size(); j++) {
			if (strcmp(names[i], layers[j].layerName)) {
				layer_found = true;
			}
		}
		if (!layer_found) {
			return false;
		}
	}

	return true;

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

bool GraphicsImpl::check_extensions_supported(const char** extensions, uint32_t extensions_count) {
	//query extensions
	uint32_t all_extensions_count;
	vkEnumerateInstanceExtensionProperties(nullptr, &all_extensions_count, nullptr);
	std::vector<VkExtensionProperties> all_extensions(all_extensions_count);
	if (all_extensions_count == 0) return false;
	vkEnumerateInstanceExtensionProperties(nullptr, &all_extensions_count, all_extensions.data());

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
