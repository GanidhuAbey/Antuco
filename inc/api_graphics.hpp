/*
------------------------ api_graphics.hpp ----------------------
 * Handles all vulkan related functionality, its important that any
 * calls made by the engine are made through the "graphics.hpp"
 * abstraction.
 * ----------------------------------------------------------------
*/

#pragma once

#include "vulkan/vulkan_core.h"
#include "window.hpp"

#include <memory>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "memory_allocator.hpp"
#include "data_structures.hpp"
#include "mesh.hpp"
#include "world_objects.hpp"
#include "config.hpp"


#include "pipeline.hpp"
#include "render_pass.hpp"

#include <vector>
#include <optional>
#include <math.h>

#ifdef NDEBUG 
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif



const uint32_t API_VERSION_1_0 = 0;

//TODO: would be nice if we had a config file/page implemented to add some tweakable values
//In case use wants many buffers it wouldn't be a good idea to create as many buffers as shadow casters
const uint32_t SHADOW_TRANSFER_BUFFERS = MAX_SHADOW_CASTERS;

const uint32_t BUFFER_SIZE = 2e8;

//CHANGING THIS IS NOT RECOMMENDED
//shader assumes that shadowmap size is 2048, i could pass in the shadowmap size into the shader but thats a waste of 
//performance for something that realistically will never need to change
const uint32_t SHADOWMAP_SIZE=4096;

const std::vector<const char*> validation_layers = {"VK_LAYER_KHRONOS_validation"};

namespace tuco {

class GraphicsImpl {
//where graphics.hpp interacts with the implementation
public:
	GraphicsImpl(Window* pWindow); //TODO: switch this to GraphicsImpl to hide vulkan and glfw
	~GraphicsImpl();

	void update_camera(glm::mat4 world_to_camera, glm::mat4 projection, glm::vec4 eye);
	void update_light(std::vector<Light*> lights, std::vector<int> shadow_casters);
	void update_draw(std::vector<GameObject*> game_objects);

private:
	glm::mat4 camera_view;
	glm::mat4 camera_projection;
    glm::vec4 camera_pos;

	std::vector<Light*> light_data; //a reference to all the light data we need.
	std::vector<int> shadow_caster_indices;

//initialize data
private:
	VkInstance instance;
	VkSurfaceKHR surface;
    std::shared_ptr<VkPhysicalDevice> p_physical_device;
    std::shared_ptr<VkDevice> p_device;
	VkDebugUtilsMessengerEXT debug_messenger;

	uint32_t graphics_family;
	uint32_t present_family;

	VkQueue graphics_queue;
	VkQueue present_queue;


//pre-configuration settings
private:
	bool raytracing;
    uint32_t oit_layers;

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
	void enable_raytracing();
	void pick_physical_device();
	void create_command_pool();
	uint32_t score_physical_device(VkPhysicalDevice physical_device);
	void destroy_initialize();

//images
private:
	std::vector<mem::Image> image_layers;

//vulkan pipelines
private:
    TucoPipeline shadowmap_pipeline;

    //if oit is enabled, then we must disable depth testing in main pipeline?
    TucoPipeline graphics_pipeline;

    //extract just the depth information from the scene.
    TucoPipeline depth_pipeline;
    //compute pipeline, draw texture based on depth texture
    TucoPipeline oit_pipeline;

private:
    void create_depth_pipeline();
    void create_oit_pipeline();

//render passes
private:
	TucoPass render_pass;
	TucoPass shadowpass;
    TucoPass geometry_pass;
    TucoPass oit_pass;

private:
    void create_oit_pass();

//draw	

private:
	VkDescriptorSetLayout ubo_layout;
	VkDescriptorSetLayout texture_layout;
    VkDescriptorSetLayout mat_layout;

	VkSwapchainKHR swapchain;
	VkExtent2D swapchain_extent;
	VkFormat swapchain_format;
	std::vector<mem::Image> swapchain_images;

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
    std::unique_ptr<mem::Pool> mat_pool;

	std::vector<std::vector<VkDescriptorSet>> ubo_sets;
    std::vector<std::vector<VkDescriptorSet>> mat_sets; //model -> mesh
	
	//game object -> mesh -> swapchain image
	std::vector<std::vector<VkDescriptorSet>> texture_sets;
	VkDescriptorSet shadowmap_set;

	VkCommandPool command_pool;
	std::vector<VkCommandBuffer> command_buffers;

	std::vector<VkDeviceSize> ubo_offsets; //holds the offset data for a objects ubo information within the uniform buffer
	std::vector<std::vector<VkDeviceSize>> mat_offsets;
	std::vector<std::vector<mem::Image>> texture_images;


	size_t current_frame = 0;
	size_t submitted_frame = 0;

	VkDescriptorSetLayout light_layout;

	bool not_created;

	//not sure why these numbers are the best
	std::vector<GameObject*>* recent_objects;

private:
    bool enable_portability = false;
    std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    
//shadow map -------------------------------------------------------------------------
private:
    mem::Image shadowmap_atlas;
	//this is way to large...
	uint32_t shadowmap_atlas_width = SHADOWMAP_SIZE*sqrt(MAX_SHADOW_CASTERS);
	uint32_t shadowmap_atlas_height = SHADOWMAP_SIZE*sqrt(MAX_SHADOW_CASTERS);

	mem::StackBuffer shadowmap_buffers[MAX_SHADOW_CASTERS];
	
	uint32_t shadowmap_width = SHADOWMAP_SIZE;
	uint32_t shadowmap_height = SHADOWMAP_SIZE;

	VkSampler shadowmap_sampler;	
	VkDescriptorSetLayout shadowmap_layout;
	
	VkFramebuffer shadowpass_buffer;

    mem::Image shadow_pass_texture;
 
	std::unique_ptr<mem::Pool> shadowmap_pool;

private:
	void create_shadowpass_buffer();
	void create_shadowpass();
	void create_shadowpass_resources();
	void create_shadowpass_pipeline();
	void create_shadowmap_transfer_buffer();
	void create_shadowmap_set();
	void create_shadowmap_layout();
	void create_shadowmap_pool();
	void create_shadowmap_sampler();
	void create_shadowmap_atlas();
	void write_to_shadowmap_set();
//-------------------------------------------------------------------------------------
//deferred shading --------------------------------------------------------------------
private: 
    VkFramebuffer g_buffer;

private:
    void create_geometry_pass();
    void create_geometry_buffer();
    void create_deffered_textures();

private:
	void create_graphics_pipeline();
	void create_ubo_layout();
	void create_ubo_pool();
	void create_ubo_set();
    void create_materials_layout();
    void create_materials_pool();
    void create_materials_set(uint32_t mesh_count);
	void create_texture_layout();
	void create_texture_pool();
	void create_texture_set(size_t mesh_count);
	void create_command_buffers(std::vector<GameObject*> game_objects);
	void create_shadow_map(std::vector<GameObject*> game_objects, size_t command_index);
	void resolve_image_layers(size_t i);
	void draw_frame();
	void create_semaphores();
	void create_fences();
	void create_swapchain();
	void create_depth_resources();
    void create_swapchain_buffers();
	void create_texture_sampler();
	void create_render_pass();
	void create_image_layers();
	VkShaderModule create_shader_module(std::vector<uint32_t> shaderCode);
	VkPipelineShaderStageCreateInfo fill_shader_stage_struct(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	void write_to_ubo();	
    void write_to_materials();
	void update_uniform_buffer(VkDeviceSize memory_offset, UniformBufferObject ubo);
    void update_materials(VkDeviceSize memory_offset, MaterialsObject mat);
    void copy_image_to_image(VkImage src_image, VkImageLayout src_layout, VkImage dst_image, VkImageLayout dst_layout, VkCommandBuffer command_buffer);
	void create_texture_image(aiString texturePath, size_t object, size_t texture_set); 
    void create_empty_image(size_t object, size_t texture_set);
	void write_to_texture_set(VkDescriptorSet texture_set, VkImageView image_view);
	void update_light_buffer(VkDeviceSize memory_offset, LightBufferObject lbo);
	void create_light_set(UniformBufferObject lbo);
	void create_light_layout();
	void cleanup_swapchain();
	void recreate_swapchain();
	void free_command_buffers();
private:
	void destroy_draw();



//helper functions
private:
    VkDescriptorBufferInfo setup_descriptor_set_buffer(uint32_t set_size);
    void update_descriptor_set(VkDescriptorBufferInfo buffer_info, uint32_t dst_binding, VkDescriptorSet set);
	void create_frame_buffer(VkRenderPass pass, uint32_t attachment_count, VkImageView* p_attachments, uint32_t width, uint32_t height, VkFramebuffer* frame_buffer);


//vulkan initialization

//graphics pipeline initialization

//buffer setup
private:
    mem::StackBuffer vertex_buffer;
	mem::StackBuffer index_buffer;
	mem::SearchBuffer uniform_buffer;
private:
	void create_uniform_buffer();
	void create_vertex_buffer();
	void create_index_buffer();

    bool check_data(size_t data_size);

	void copy_buffer(mem::Memory src_buffer, mem::Memory dst_buffer, VkDeviceSize dst_offset, VkDeviceSize data_size);
public:
	int32_t update_vertex_buffer(std::vector<Vertex> vertex_data);
	int32_t update_index_buffer(std::vector<uint32_t> indices_data);

//draw commands

};

}
