#include "pass.hpp"

#include <vulkan/vulkan.hpp>
#include <cstdint>

namespace pass {

class ComputePass : Pass {
public:
	void frame_begin();
	void build_command_list(const std::vector<std::unique_ptr<vk::CommandBuffer>>& command_buffers);

	// Add resources
	void add_resources(vk::Buffer* buffer);
	void add_resources(vk::Image* image);

private:
	uint32_t thread_size;
	vk::Queue queue;
	
};

}