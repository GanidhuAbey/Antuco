#include "api_graphics.hpp"
#include "logger/interface.hpp"
#include "queue_data.hpp"

#include <stdint.h>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <map>
#include <set>

#include "api_config.hpp"

using namespace tuco;


void GraphicsImpl::destroy_initialize() {	
}

//checks for and enables required raytracing extensions
//crashes if raytracing is enabled on unsupported GPU.
void GraphicsImpl::enable_raytracing() {

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

