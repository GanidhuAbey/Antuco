#include "passes/compute_pass.hpp"

using namespace pass;

ComputePass::ComputePass(
v::PhysicalDevice& phys_device, v::Surface& surface) {
	: Pass(phys_device, surface)	
}

void ComputePass::build_command_list(
const std::vector<std::unique_ptr<vk::CommandBuffer>>& command_buffers) {
	 
}
