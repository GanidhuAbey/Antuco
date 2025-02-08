/*
------------------------ api_graphics.hpp ----------------------
 * Handles all vulkan related functionality, its important that any
 * calls made by the engine are made through the "graphics.hpp"
 * abstraction.
 * ----------------------------------------------------------------
*/

#pragma once

#include <vulkan/vulkan.hpp>

#include "material.hpp"
#include "window.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "config.hpp"
#include "data_structures.hpp"
#include "descriptor_set.hpp"
#include "memory_allocator.hpp"
#include "mesh.hpp"
#include "world_objects.hpp"
#include <scene.hpp>

#include "vulkan_wrapper/device.hpp"
#include "vulkan_wrapper/instance.hpp"
#include "vulkan_wrapper/physical_device.hpp"
#include "vulkan_wrapper/surface.hpp"
#include "vulkan_wrapper/swapchain.hpp"

#include <bedrock/image.hpp>
#include <bedrock/draw_item.hpp>

#include "pipeline.hpp"
#include "render_pass.hpp"

#include <math.h>
#include <optional>
#include <vector>

const uint32_t API_VERSION_1_0 = 0;

// TODO: pass parameter values through config file, prevent
//       recompilation
const uint32_t SHADOW_TRANSFER_BUFFERS = MAX_SHADOW_CASTERS;

const uint32_t BUFFER_SIZE = 2e8;

// CHANGING THIS IS NOT RECOMMENDED
const uint32_t SHADOWMAP_SIZE = 2048;

const std::vector<const char *> validation_layers = {
    "VK_LAYER_KHRONOS_validation"};

namespace tuco {

class GraphicsImpl {
    // where graphics.hpp interacts with the implementation
public:
    GraphicsImpl(Window *pWindow); // TODO: switch this to GraphicsImpl to hide
                                    // vulkan and glfw
    ~GraphicsImpl();

    void update_camera(glm::mat4 world_to_camera, glm::mat4 projection,
                        glm::vec4 eye);
    void update_light(std::vector<DirectionalLight> lights,
                    std::vector<int> shadow_casters);
    void update_draw(std::vector<std::unique_ptr<GameObject>> &game_objects);

    void initialize_scene(SceneData *scene);

    std::shared_ptr<mem::Pool> get_set_pool() { return set_pool; }
    vk::CommandPool& get_command_pool() { return command_pool; }
    mem::StackBuffer& get_vertex_buffer() { return vertex_buffer; }
    mem::StackBuffer& get_index_buffer() { return index_buffer; }
    mem::SearchBuffer& get_model_buffer() { return uniform_buffer; }


private:
    glm::mat4 camera_view;
    glm::mat4 camera_projection;
    glm::vec4 camera_pos;

    std::vector<DirectionalLight>
        light_data; // a reference to all the light data we need.
    std::vector<int> shadow_caster_indices;

    // initialize data
public:
    std::shared_ptr<v::Instance> p_instance;
    std::shared_ptr<v::Surface> p_surface;
    std::shared_ptr<v::PhysicalDevice> p_physical_device;
    std::shared_ptr<v::Device> p_device;
    v::Swapchain swapchain;
    VkDebugUtilsMessengerEXT debug_messenger;

#ifdef APPLE_M1
    bool print_debug = false;
#else
    bool print_debug = true;
#endif

    // pre-configuration settings
private:
    bool raytracing;
    uint32_t oit_layers;

private:
    VkResult create_debug_utils_messengar_utils(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger);

    bool validation_layer_supported(std::vector<const char *> names);
    bool check_extensions_supported(const char **extensions,
                                    uint32_t extensions_count);
    bool check_device_extensions(std::vector<const char *> extensions,
                                uint32_t extensions_count);

private:
    void create_instance(const char *appName);
    void create_logical_device();
    void enable_raytracing();
    void pick_physical_device();
    uint32_t score_physical_device(VkPhysicalDevice physical_device);
    void destroy_initialize();

    // images
private:
    std::vector<br::Image> output_images;
    // vulkan pipelines
private:
    TucoPipeline shadowmap_pipeline;

    TucoPipeline screen_pipeline;

    // if oit is enabled, then we must disable depth testing in main pipeline?
    std::vector<TucoPipeline> graphics_pipelines;
    TucoPipeline skybox_pipeline;

private:
    void create_depth_pipeline();
    void create_oit_pipeline();
    void create_screen_pipeline();

    // render passes
private:
    TucoPass render_pass;
    TucoPass skybox_pass;
    TucoPass screen_pass;
    TucoPass shadowpass;
    TucoPass oit_pass;

private:
    void create_oit_pass();
    void create_screen_pass();

    // draw
public:
    std::vector<br::DrawItem> draw_items;
    std::vector<Material> materials;

    // what we really want is a unique pointer that safe for vectors
    std::vector<std::unique_ptr<br::GPUResource>> draw_data;

    // creating draw packet from object data is fine, but it needs to be within pass?
    void update_draw_item(uint32_t object_index, tuco::GameObject& object);

    uint32_t add_material();
    uint32_t add_draw_data(br::GPUResource* resource);

private:
    VkDescriptorSetLayout ubo_layout;
    VkDescriptorSetLayout texture_layout;
    VkDescriptorSetLayout matLayout;
    VkDescriptorSetLayout light_layout;
    VkDescriptorSetLayout scene_layout;

    ResourceCollection scene_collection;
    ResourceCollection materialCollection;
    MaterialGpuInfo globalMaterialOffsets;

    vk::Sampler texture_sampler;

    std::vector<VkFramebuffer> output_buffers;
    std::vector<VkFramebuffer> screen_buffers;

    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;

    std::vector<VkFence> in_flight_fences;
    std::vector<VkFence> images_in_flight;

    std::vector<ResourceCollection> light_ubo;
    std::vector<uint32_t> light_offsets;

    br::Image depth_image;

    // this will be the shadowmap atlas that will contain all the shadowmap data
    // for the scene

    std::unique_ptr<mem::Pool> ubo_pool;
    std::unique_ptr<mem::Pool> texture_pool;
    std::unique_ptr<mem::Pool> matPool;

    // pool for all sets.
    std::shared_ptr<mem::Pool> set_pool;

    std::vector<std::vector<VkDescriptorSet>> uboSets;
    std::vector<std::vector<VkDescriptorSet>> matSets; // model -> mesh

    // game object -> mesh -> swapchain image
    std::vector<ResourceCollection> texture_sets;
    VkDescriptorSet shadowmap_set;
    ResourceCollection screen_resource;

    vk::CommandPool command_pool;
    std::vector<vk::CommandBuffer> command_buffers;

    std::vector<VkDeviceSize>
        ubo_offsets; // holds the offset data for a objects ubo information within
                    // the uniform buffer
    std::vector<VkDeviceSize> matOffsets;
    std::vector<std::vector<br::Image>> texture_images;

    br::Image uninitalized_image;

    size_t current_frame = 0;
    size_t submitted_frame = 0;

    bool not_created;

    // not sure why these numbers are the best
    bool update_command_buffers = false;

private:
    bool enable_portability = false;
    std::vector<const char *> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // shadow map
    // -------------------------------------------------------------------------
private:
    // this is way to large...
    uint32_t shadowmap_atlas_width = SHADOWMAP_SIZE * sqrt(MAX_SHADOW_CASTERS);
    uint32_t shadowmap_atlas_height = SHADOWMAP_SIZE * sqrt(MAX_SHADOW_CASTERS);

    mem::StackBuffer shadowmap_buffers[MAX_SHADOW_CASTERS];

    uint32_t shadowmap_width = SHADOWMAP_SIZE;
    uint32_t shadowmap_height = SHADOWMAP_SIZE;

    float depth_bias_constant = 3.5f;
    float depth_bias_slope = 9.5f;

    //vk::Sampler shadowmap_sampler;
    //VkDescriptorSetLayout shadowmap_layout;

    vk::Framebuffer shadowpass_buffer;

    //mem::Image shadow_pass_texture;

    std::unique_ptr<mem::Pool> shadowmap_pool;

private:
    //void create_shadowpass_buffer();
    void create_shadowpass();
    //void create_shadowpass_resources();
    void create_shadowpass_pipeline();
    //void create_shadowmap_transfer_buffer();
    void create_shadowmap_set();
    void create_shadowmap_layout();
    void create_shadowmap_pool();
    void create_shadowmap_sampler();
    void create_shadowmap_atlas();
    //void write_to_shadowmap_set();
    //-------------------------------------------------------------------------------------
    // deferred shading
    // --------------------------------------------------------------------
private:
    VkFramebuffer g_buffer;

private:
    //void create_geometry_pass();
    void create_geometry_buffer();
    void create_deffered_textures();
    
public:
    void render_to_screen(size_t i, vk::CommandBuffer command_buffer);
    void write_screen_set();

private:
    void create_skybox_pipeline();
    void create_graphics_pipeline();
    void create_ubo_layout();
    void create_ubo_pool();
    void createUboSets(uint32_t setCount);
    void createMaterialLayout();
    void createMaterialPool();
    void createMaterialCollection();
    void create_set_pool();
    void create_pools();
    void create_scene_collection();
    void create_scene_layout();
    void create_texture_layout();
    void create_texture_pool();
    void create_texture_set(size_t mesh_count);
    void create_command_buffers(
        const std::vector<std::unique_ptr<GameObject>> &game_objects, SceneData* scene);

    void create_shadow_map(
        const std::vector<std::unique_ptr<GameObject>> &game_objects,
        size_t command_index, LightObject light);


    void copy_to_swapchain(size_t i);

    std::vector<VkDescriptorSet> create_set(VkDescriptorSetLayout layout,
                                            size_t set_count, mem::Pool &pool);

    void create_screen_set();
    void draw_frame();
    void create_semaphores();
    void create_fences();
    void create_depth_resources();
    void create_output_buffers();
    void create_screen_buffer();
    void create_texture_sampler();

    void create_render_pass();
    void create_skybox_pass();
    
    void create_output_images();
    VkShaderModule create_shader_module(std::vector<uint32_t> shaderCode);

    VkPipelineShaderStageCreateInfo
    fill_shader_stage_struct(VkShaderStageFlagBits stage,
                            VkShaderModule shaderModule);

    void write_to_ubo();
    MaterialGpuInfo setupMaterialBuffers();
    void updateMaterialResources(Material &material);
    void write_scene(SceneData* scene);
    void create_scene(SceneData* scene);
    void writeMaterial(Material &material);
    void writeSceneCollection(SceneData &scene);

    void update_uniform_buffer(VkDeviceSize memory_offset,
                                UniformBufferObject ubo);

    void updateUniformBuffer(VkDeviceSize offset, VkDeviceSize size, void *data);
    void update_materials(VkDeviceSize memory_offset, Material mat);
    void copy_image_to_image(VkImage src_image, VkImageLayout src_layout,
                            VkImage dst_image, VkImageLayout dst_layout,
                            VkCommandBuffer command_buffer);

    void create_vulkan_image(const ImageBuffer &image, size_t i, size_t j);
    void create_texture_image(std::string texturePath, size_t object,
                            size_t texture_set);
    void create_default_images();
    void write_to_texture_set(ResourceCollection texture_set, br::Image image);
    void update_light_buffer(VkDeviceSize memory_offset, LightBufferObject lbo);
    void create_light_set(uint32_t set_count);
    void create_light_layout();
    void cleanup_swapchain();
    void recreate_swapchain();
    void free_command_buffers();

private:
    void destroy_draw();

  // helper functions
private:
    VkDescriptorBufferInfo allocateDescriptorBuffer(uint32_t size);
    void update_descriptor_set(VkDescriptorBufferInfo buffer_info,
                                uint32_t dst_binding, VkDescriptorSet set);
    vk::Framebuffer create_frame_buffer(vk::RenderPass pass,
                                        uint32_t attachment_count,
                                        vk::ImageView *p_attachments,
                                        uint32_t width, uint32_t height);
    void memory_dependency(
        size_t i, VkAccessFlags src_a = VK_ACCESS_MEMORY_WRITE_BIT,
        VkAccessFlags dst_a = VK_ACCESS_MEMORY_READ_BIT,
        VkPipelineStageFlags src_p = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        VkPipelineStageFlags dst_p = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

  // vulkan initialization

  // graphics pipeline initialization

  // buffer setup
private:
    // [TODO 08/24] - Should obscure the underlying implementation of buffer by adding interface on top (allow extending to more buffer types)
    mem::StackBuffer vertex_buffer;
    mem::StackBuffer index_buffer;
    mem::SearchBuffer uniform_buffer;

private:
    void create_uniform_buffer();
    void create_vertex_buffer();
    void create_index_buffer();

    bool check_data(size_t data_size);

    void copy_buffer(mem::Memory src_buffer, mem::Memory dst_buffer,
                    VkDeviceSize dst_offset, VkDeviceSize data_size);

public:
    int32_t update_vertex_buffer(std::vector<Vertex> vertex_data);
    int32_t update_index_buffer(std::vector<uint32_t> indices_data);

  // draw commands
};

} // namespace tuco
