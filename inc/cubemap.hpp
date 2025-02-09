#pragma once

#include <string>

#include <bedrock/image.hpp>
#include <pipeline.hpp>
#include <render_pass.hpp>
#include <vulkan_wrapper/framebuffer.hpp>
#include <memory_allocator.hpp>

#include <world_objects.hpp>

#define INDEX(face, mip, max_mip) (face * max_mip) + mip

namespace tuco {

class Cubemap {
protected:
	std::string name;

	TucoPipeline pipeline;
	TucoPass pass;
	std::vector<v::Framebuffer> outputs;
	std::vector<vk::ImageView> faces;

	br::Image cubemap;
	uint32_t map_size;
	uint32_t mip_count;

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
	void init(std::string name, std::string& vert, std::string& frag, GameObject* model, uint32_t size, uint32_t mip_count = 1);
	void set_input(br::Image* image);
	

	br::Image& get_image() { return cubemap; }

	~Cubemap();

	virtual void record_command_buffer(uint32_t face, VkCommandBuffer command_buffer);

protected:
	void create_pipeline(std::string& vert, std::string& frag);
	virtual void override_pipeline(PipelineConfig& config) {};

	void create_pass();
	void create_cubemap_faces();
	void create_framebuffers();
	void write_descriptors();
};

}