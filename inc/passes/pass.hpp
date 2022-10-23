#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace pass {

class Pass {
private:
	void init(const std::vector<vk::CommandBuffer>& command_buffers);

protected:
	std::vector<vk::CommandBuffer>& command_buffers;

};

}