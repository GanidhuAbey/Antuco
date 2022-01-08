//TODO: change all iterations of vec3 to vec4 in lightobject and see if it works

#pragma once

#include "window.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "memory_allocator.hpp"
#include "data_structures.hpp"
#include "mesh.hpp"
#include "world_objects.hpp"
#include "config.hpp"

#include <vector>
#include <math.h>

#ifdef NDEBUG 
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//TODO: would be nice if we had a config file/page implemented to add some tweakable values
//In case use wants many buffers it wouldn't be a good idea to create as many buffers as shadow casters
const uint32_t SHADOW_TRANSFER_BUFFERS = MAX_SHADOW_CASTERS;

const uint32_t BUFFER_SIZE = 2e7;

//CHANGING THIS IS NOT RECOMMENDED
//shader assumes that shadowmap size is 2048, i could pass in the shadowmap size into the shader but thats a waste of 
//performance for something that realistically will never need to change
const uint32_t SHADOWMAP_SIZE=2048;

const std::vector<const char*> validation_layers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

namespace tuco {

class GraphicsImpl {
//where graphics.hpp interacts with the implementation
public:
	GraphicsImpl(Window* pWindow); //TODO: switch this to GraphicsImpl to hide vulkan and glfw
	~GraphicsImpl();

	void update_camera(glm::mat4 world_to_camera, glm::mat4 projection);
	void update_light(std::vector<Light*> lights, std::vector<int> shadow_casters);
	void update_draw(std::vector<GameObject*> game_objects);

private:
	glm::mat4 camera_view;
	glm::mat4 camera_projection;

	std::vector<Light*> light_data; //a reference to all the light data we need.
	std::vector<int> shadow_caster_indices;

//initialize data
private:
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkDebugUtilsMessengerEXT debug_messenger;

	uint32_t graphics_family;
	uint32_t present_family;

	VkQueue graphics_queue;
	VkQueue present_queue;

private:
	VkResult create_debug_utils_messengar_utils(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);
	bool validation_layer_supported(std::vector<const char*> names);
	bool check_extensions_supported(const char** extensions, uint32_t extensions_count);
	bool check_device_extensions(std::vector<const char*> extensions, uint32_t extensions_count);
	
private:
	void create_instance(const char* appName);
	void create_logical_device();
	void pick_physical_device();
	void create_command_pool();
	uint32_t score_physical_device(VkPhysicalDevice physical_device);
	void destroy_initialize();


//draw

//shadow map data
private:
	mem::Memory shadowmap_atlas;
	//this is way to large...
	uint32_t shadowmap_atlas_width = SHADOWMAP_SIZE*sqrt(MAX_SHADOW_CASTERS);
	uint32_t shadowmap_atlas_height = SHADOWMAP_SIZE*sqrt(MAX_SHADOW_CASTERS);

	mem::StackBuffer shadowmap_buffers[MAX_SHADOW_CASTERS];
	
	uint32_t shadowmap_width = SHADOWMAP_SIZE;
	uint32_t shadowmap_height = SHADOWMAP_SIZE;

	VkSampler shadowmap_sampler;	
	VkDescriptorSetLayout shadowmap_layout;

	VkRenderPass shadowpass;

	VkPipeline shadowpass_pipeline;

	VkPipelineLayout shadowpass_layout;
	
	VkFramebuffer shadowpass_buffer;

	mem::Memory shadow_pass_texture;
	

private:
	VkDescriptorSetLayout ubo_layout;
	VkDescriptorSetLayout texture_layout;

	VkSwapchainKHR swapchain;
	VkExtent2D swapchain_extent;
	VkFormat swapchain_format;
	std::vector<VkImage> swapchain_images;
	std::vector<VkImageView> swapchain_image_views;

	VkRenderPass render_pass;

	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;


	VkSampler texture_sampler;

	std::vector<VkFramebuffer> swapchain_framebuffers;

	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;

	std::vector<VkFence> in_flight_fences;
	std::vector<VkFence> images_in_flight;

	std::vector<std::vector<VkDescriptorSet>> light_ubo;
	std::vector<uint32_t> light_offsets;

	mem::Memory depth_memory;

	//this will be the shadowmap atlas that will contain all the shadowmap data for the scene

	std::unique_ptr<mem::Pool> ubo_pool;
	std::unique_ptr<mem::Pool> texture_pool;
	std::unique_ptr<mem::Pool> shadowmap_pool;

	std::vector<std::vector<VkDescriptorSet>> ubo_sets;
	
	//game object -> mesh -> swapchain image
	std::vector<std::vector<std::vector<VkDescriptorSet>>> texture_sets;
	VkDescriptorSet shadowmap_set;

	VkCommandPool command_pool;
	std::vector<VkCommandBuffer> command_buffers;

	std::vector<VkDeviceSize> ubo_offsets; //holds the offset data for a objects ubo information within the uniform buffer
	std::vector<std::vector<mem::Memory*>> texture_images;


	size_t current_frame = 0;
	size_t submitted_frame = 0;

	VkDescriptorSetLayout light_layout;

	bool not_created;

	//not sure why these numbers are the best
	std::vector<GameObject*>* recent_objects;

private:
	void create_graphics_pipeline();
	void create_ubo_layout();
	void create_ubo_pool();
	void create_ubo_set();
	void create_texture_layout();
	void create_texture_pool();
	void create_texture_set(size_t mesh_count);
	void create_command_buffers(std::vector<GameObject*> game_objects);
	void draw_frame();
	void create_semaphores();
	void create_fences();
	void create_swapchain();
	void create_depth_resources();
	void create_frame_buffers();
	void create_shadowpass_buffer();
	void create_colour_image_views();
	void create_texture_sampler();
	void create_render_pass();
	void create_shadowpass();
	void create_shadowpass_resources();
	void create_shadowpass_pipeline();
	VkShaderModule create_shader_module(std::vector<char> shaderCode);
	std::vector<char> read_file(const std::string& filename);
	VkPipelineShaderStageCreateInfo fill_shader_stage_struct(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	void write_to_ubo();	
	void update_uniform_buffer(VkDeviceSize memory_offset, UniformBufferObject ubo);
	void copy_buffer_to_image(VkBuffer buffer, mem::Memory image, VkOffset3D image_offset, VkImageAspectFlagBits aspect_mask, uint32_t image_width, uint32_t image_height, std::optional<VkCommandBuffer> command_buffer = std::nullopt);	
	void copy_image_to_buffer(VkBuffer buffer, mem::Memory image, VkImageLayout image_layout, VkImageAspectFlagBits image_aspect, VkDeviceSize dst_offset, std::optional<VkCommandBuffer> command_buffer=std::nullopt);
	void transfer_image_layout(VkImageLayout initial_layout, VkImageLayout output_layout, VkImage image, VkImageAspectFlagBits aspect_mask, std::optional<VkCommandBuffer> command_buffer=std::nullopt);
	void create_shadowmap_transfer_buffer();
	void create_texture_image(aiString texturePath, size_t object, size_t texture_set);
	void write_to_texture_set(std::vector<VkDescriptorSet> texture_sets, mem::Memory* image);
	void update_light_buffer(VkDeviceSize memory_offset, LightBufferObject lbo);
	void create_shadowmap_set();
	void create_shadowmap_layout();
	void create_shadowmap_pool();
	void create_shadowmap_sampler();
	void create_shadowmap_atlas();
	void write_to_shadowmap_set();
	void create_light_set(UniformBufferObject lbo);
	void create_light_layout();
	void cleanup_swapchain();
	void recreate_swapchain();
	void free_command_buffers();
private:
	void destroy_draw();


//vulkan initialization

//graphics pipeline initialization

//buffer setup
private:
	mem::Memory vertex_buffer;
	mem::Memory index_buffer;
	mem::SearchBuffer uniform_buffer;
private:
	void create_uniform_buffer();
	void create_vertex_buffer();
	void create_index_buffer();

	void copy_buffer(mem::Memory src_buffer, mem::Memory dst_buffer, VkDeviceSize dst_offset, VkDeviceSize data_size);
	VkCommandBuffer begin_command_buffer();
	void end_command_buffer(VkCommandBuffer command_buffer);
public:
	void update_vertex_buffer(std::vector<Vertex> vertex_data);
	void update_index_buffer(std::vector<uint32_t> indices_data);

//draw commands

};

}
