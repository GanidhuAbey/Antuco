#pragma once

#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"

extern VkCommandBuffer begin_command_buffer(VkDevice device, VkCommandPool command_pool);


extern void end_command_buffer(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkCommandBuffer commandBuffer);
