#include "pass.hpp"

#include <vulkan_wrapper/physical_device.hpp>
#include <vulkan_wrapper/surface.hpp>
#include <vulkan_wrapper/queue.hpp>
#include <vulkan/vulkan.hpp>
#include <cstdint>

namespace pass {

class ComputePass : Pass {
public:
	ComputePass(v::PhysicalDevice& phys_device, v::Surface& surface);	

private:
	uint32_t thread_size;	
};

}
