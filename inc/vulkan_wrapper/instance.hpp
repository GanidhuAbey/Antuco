#pragma once

#include <vkwr.hpp>

#include <GLFW/glfw3.h>

namespace v {
class Instance {
private:
    std::vector<const char*> validation_layers = {"VK_LAYER_KHRONOS_validation"};
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT messenger;
    bool enable_validation = true;

    vk::DispatchLoaderDynamic m_dldi;

public:
    Instance(std::string app_name, uint32_t api_version);
    ~Instance();

    vk::Instance get() { return instance; }

    operator vk::Instance() { return instance; }
    operator VkInstance() { return instance; }

    vk::DispatchLoaderDynamic& get_dldi() { return m_dldi; }

private:
    void create_instance(const char* app_name, uint32_t api_version);
    VkResult create_debug_utils_messengar_utils(
                            VkInstance instance,
	                        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	                        const VkAllocationCallbacks* pAllocator,
	                        VkDebugUtilsMessengerEXT* pDebugMessenger);

	bool check_extensions_supported(const char** extensions, uint32_t extensions_count);
    bool validation_layer_supported(std::vector<const char*> names);

};
}
