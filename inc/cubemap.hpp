#pragma once

#include <string>

#include <bedrock/image.hpp>
#include <pipeline.hpp>
#include <render_pass.hpp>
#include <vulkan_wrapper/framebuffer.hpp>
#include <memory_allocator.hpp>

#include <world_objects.hpp>

namespace tuco {

class Cubemap {
private:
	TucoPipeline pipeline;
	TucoPass pass;
	std::vector<v::Framebuffer> outputs;
	std::vector<vk::ImageView> faces;

	br::Image cubemap;

	br::Image* input_image;

	std::shared_ptr<v::Device> device_;
	std::shared_ptr<v::PhysicalDevice> physical_device_;
	std::shared_ptr<mem::Pool> set_pool_;

	GameObject* cubemap_model; // a ptr to a cube model that we can use to render the hdr image onto.
	std::vector<uint32_t> ubo_buffer_offsets; // offset to ubo data in uniform buffer. 1 offset per every cubemap face.

public:
	// the provided path should be to a folder containing all the skybox images
	// images should be named such that:
	//	"nx" - negative x image
	//	"px" - positive x image
	void init(std::string& vert, std::string& frag, GameObject * model);
	void set_input(br::Image* image);
	

	br::Image& get_image() { return cubemap; }

	~Cubemap();

	void record_command_buffer(uint32_t face, VkCommandBuffer command_buffer);

private:
	void create_pipeline(std::string& vert, std::string& frag);
	void create_pass();
	void create_cubemap_faces();
	void create_framebuffers();
	void write_descriptors();
};

}