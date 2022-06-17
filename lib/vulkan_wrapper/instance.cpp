#include "vulkan_wrapper/instance.hpp"
#include "vulkan/vulkan_core.h"

#include <cstdio>
#include <iostream>

using namespace v;

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	//arrow is used in pointers of objects/structs
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}


Instance::Instance(std::string app_name, uint32_t api_version) {
    create_instance(app_name.c_str(), api_version);
}

Instance::~Instance() {

}


bool Instance::validation_layer_supported(std::vector<const char*> names) {
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


void Instance::create_instance(const char* app_name, uint32_t api_version) {  
#ifdef NDEBUG 
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

    vk::ApplicationInfo app_info(app_name, 1, "Antuco", 1, api_version);

    std::vector<const char*> layers;
    void* next = nullptr;

    vk::DebugUtilsMessengerCreateInfoEXT debug_info(
            {}, 
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | 
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError, 
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | 
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance, 
            &debug_callback);

	//determine layer count (we only really care about the debug validation layers)
	if (enableValidationLayers && validation_layer_supported(validation_layers)) {
		printf("[DEBUG] - VALIDATION LAYERS ENABLED \n");	
        layers = validation_layers;
		next = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_info;
	}
	else {
		if (enableValidationLayers) printf("[WARNING] - could not enable validation layers \n");
    }
    
	//determine extensions
	uint32_t glfw_extension_count;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#ifdef APPLE_M1
	extensions.push_back("VK_KHR_get_physical_device_properties2");
#else
    /*
	if (raytracing) {
		extensions.push_back("VK_KHR_get_physical_device_properties2");
	}
    */
#endif

    std::vector<const char*> enabled_extensions;
	
	if (check_extensions_supported(extensions.data(), static_cast<uint32_t>(extensions.size()))) {
		enabled_extensions = extensions;
	}
	else {
		printf("[ERROR] - required instance extensions not supported \n");
		throw std::runtime_error("");
	}

    vk::InstanceCreateInfo info(
            {}, 
            &app_info, 
            layers.size(), layers.data(), 
            enabled_extensions.size(), enabled_extensions.data());

    instance = vk::createInstance(info); 

    auto dldi = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);

    messenger = instance.createDebugUtilsMessengerEXT(debug_info, nullptr, dldi);
}

bool Instance::check_extensions_supported(const char** extensions, uint32_t extensions_count) {
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
