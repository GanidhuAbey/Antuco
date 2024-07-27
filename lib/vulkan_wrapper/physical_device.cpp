#include "vulkan_wrapper/physical_device.hpp"
#include "vulkan_wrapper/limits.hpp"

using namespace v;

PhysicalDevice::PhysicalDevice(std::shared_ptr<v::Instance> instance) {
	m_instance = instance;
    pick_physical_device(instance.get());
}

PhysicalDevice::~PhysicalDevice() {}

void PhysicalDevice::pick_physical_device(Instance* instance) {
#ifdef NDEBUG 
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
	//query all physical devices we have
    std::vector<vk::PhysicalDevice> phys_devices = instance->get().enumeratePhysicalDevices();

	//score each device
	std::multimap<uint32_t, vk::PhysicalDevice> device_score;
	for (const auto& current_device : phys_devices) {
		uint32_t score = score_physical_device(current_device);
		device_score.insert(std::make_pair(score, current_device));
	}

	//pick device with highest score
	uint32_t biggest = 0;
    vk::PhysicalDevice current_best_device;
    bool found = false;
	for (auto i = device_score.rbegin(); i != device_score.rend(); i++) {
		if (i->first > biggest) {
			biggest = i->first;
			current_best_device = i->second;
            found = true;
		}
	}
	if (!found) {
		printf("[ERROR] - could not find suitable GPU from this device");
		throw std::runtime_error("");
	}
	
	if (enableValidationLayers) {
        vk::PhysicalDeviceProperties properties;
        properties = current_best_device.getProperties();
		
		printf("[DEBUG] - CHOSEN GPU: %s \n", 
        
		properties.deviceName.data());

		printf("[DEBUG] - MAXIMUM PUSH CONSTANT RANGE: %u \n", 
                properties.limits.maxPushConstantsSize);
	}

	//make the best physical device the one we'll use for the program
	physical_device = current_best_device;

	set_device_limits();
}

void PhysicalDevice::set_device_limits() {
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &device_properties);

	Limits::get().uniformBufferOffsetAlignment = device_properties.limits.minUniformBufferOffsetAlignment;
}

uint32_t PhysicalDevice::score_physical_device(vk::PhysicalDevice physical_device) {
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

