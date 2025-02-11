#pragma once

#include <string>

#include <bedrock/image.hpp>
#include <pipeline.hpp>
#include <render_pass.hpp>
#include <vulkan_wrapper/framebuffer.hpp>
#include <memory_allocator.hpp>

#include <cubemap.hpp>
#include <environment/prefilter_map.hpp>
#include <lut.hpp>

#include <world_objects.hpp>

namespace tuco {

class Environment {
private:
	br::Image input_image;

	Cubemap skybox;
	Cubemap irradiance_map;
	PrefilterMap specular_map;
	LUT brdf_map;

	mem::CommandPool command_pool_;
	std::vector<VkCommandBuffer> command_buffers;
	std::shared_ptr<v::Device> p_device;

	std::vector<VkSemaphore> gpu_sync;
	std::vector<VkFence> cpu_sync;

public:
	// provide the path to the hdr image from which the environment maps are generated from.
	void init(std::string path, GameObject* model);

	Cubemap& get_environment() { return skybox; }
	Cubemap& get_irradiance() { return irradiance_map; }
	Cubemap& get_specular() { return specular_map; }
	LUT& get_brdf() { return brdf_map; }

	~Environment();

private:
	void record_command_buffers();
	void render_to_image();
};

}