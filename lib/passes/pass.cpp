#include "passes/pass.hpp"

using namespace tuco;

void Pass::init(const std::vector<vk::CommandBuffer>& command_buffers) {
	Pass::command_buffers = command_buffers;
}