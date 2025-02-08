#pragma once

#include <string>

#include <bedrock/image.hpp>
#include <pipeline.hpp>
#include <render_pass.hpp>
#include <vulkan_wrapper/framebuffer.hpp>
#include <memory_allocator.hpp>

#include <world_objects.hpp>

namespace tuco {

class Environment {
private:
	TucoPipeline skybox_pipeline;
	TucoPass skybox_pass;
	std::vector<v::Framebuffer> skybox_outputs;
	std::vector<vk::ImageView> cubemap_faces;

	br::Image cubemap;

	br::Image hdr_image;

	std::shared_ptr<v::Device> device_;
	std::shared_ptr<v::PhysicalDevice> physical_device_;
	std::shared_ptr<mem::Pool> set_pool_;
	mem::CommandPool command_pool_;

	GameObject* cubemap_model; // a ptr to a cube model that we can use to render the hdr image onto.
	std::vector<uint32_t> ubo_buffer_offsets; // offset to ubo data in uniform buffer. 1 offset per every cubemap face.

	// render synchronization
	std::vector<VkSemaphore> gpu_sync;
	std::vector<VkFence> cpu_sync;

	std::vector<VkCommandBuffer> command_buffers;

public:
	// the provided path should be to a folder containing all the skybox images
	// images should be named such that:
	//	"nx" - negative x image
	//	"px" - positive x image
	void init(std::string path, GameObject* model);

	br::Image& get_image() { return cubemap; }

	~Environment();

private:
	void create_skybox_pipeline();
	void create_skybox_pass();
	void create_cubemap_faces();
	void create_skybox_framebuffers();
	void write_descriptors();

	void record_command_buffers();
	void render_to_image();
};

}