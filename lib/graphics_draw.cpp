// this will handle settingcl up the graphics pipeline, recording command
// buffers, and generally anything requiring rendering within the vulkan
// framework will end up here
#include "api_graphics.hpp"

#include "data_structures.hpp"
#include "descriptor_set.hpp"
#include "material.hpp"
#include "memory_allocator.hpp"

#include <vulkan_wrapper/limits.hpp>
#include "antuco.hpp"

#include "queue.hpp"

#include "logger/interface.hpp"
#include "vulkan/vulkan_core.h"

#include <bedrock/shader_text.hpp>

#include <stb_image.h>

#include <GLFW/glfw3.h>
#include <array>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

#include "api_config.hpp"

#include <stdexcept>

#include <glm/ext.hpp>

#define TIME_IT std::chrono::high_resolution_clock::now();

// TODO: this isn't a great method considering if we move around our files
//       the whole thing will break...
const std::string SHADER_PATH = get_project_root(__FILE__) + "/shaders/";

#define SHADER(file) SHADER_PATH + file

const uint32_t MAX_FRAMES_IN_FLIGHT = 3;

using namespace tuco;

void GraphicsImpl::create_depth_pipeline()
{
	PipelineConfig config{};
	config.vert_shader_path = SHADER_PATH + "shader.vert";
	config.blend_colours = false;
//	config.descriptor_layouts = { light_layout, ubo_layout };

	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	config.dynamic_states = dynamic_states;
	// config.pass
}

/*
/// <summary>
/// Creates a large image to contain the depth
/// textures of all the lights in the scene
/// </summary>
void GraphicsImpl::create_shadowmap_atlas() {
  mem::ImageCreateInfo image_info{};
  image_info.extent =
	  vk::Extent3D(shadowmap_atlas_width, shadowmap_atlas_height, 1);
  image_info.format = vk::Format::eD16Unorm;
  image_info.usage =
	  vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
  image_info.queueFamilyIndexCount = 1;
  image_info.pQueueFamilyIndices = &p_device->get_graphics_family();

  VkImageViewUsageCreateInfo usageInfo{};
  usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
  usageInfo.usage =
	  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  // setup create struct for image views
  mem::ImageViewCreateInfo createInfo{};
  createInfo.pNext = &usageInfo;
  createInfo.aspect_mask = vk::ImageAspectFlagBits::eDepth;

  mem::ImageData data{};
  data.name = "shadowmap";
  data.image_info = image_info;
  data.image_view_info = createInfo;

  shadowmap_atlas.init(p_physical_device, p_device, data);
}
*/

void GraphicsImpl::create_texture_sampler()
{
	auto sampler_info = vk::SamplerCreateInfo(
		{}, vk::Filter::eNearest, vk::Filter::eNearest,
		vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge, 0.0f, false, 0.0f, false,
		vk::CompareOp::eNever, 0.0f, 0.0f, vk::BorderColor::eFloatOpaqueBlack,
		false);

	texture_sampler = p_device->get().createSampler(sampler_info);
}

//void GraphicsImpl::create_shadowmap_sampler() {
//  auto sampler_info = vk::SamplerCreateInfo(
//      {}, vk::Filter::eNearest, vk::Filter::eNearest,
//      vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge,
//      vk::SamplerAddressMode::eClampToEdge,
//      vk::SamplerAddressMode::eClampToEdge, 0.0f, false, 0.0f, false,
//      vk::CompareOp::eNever, 0.0f, 0.0f, vk::BorderColor::eFloatOpaqueBlack,
//      false);
//
//  shadowmap_sampler = p_device->get().createSampler(sampler_info);
//}

//void GraphicsImpl::create_shadowpass_buffer() {
//  vk::ImageView image_views[1] = {shadow_pass_texture.get_api_image_view()};
//
//  shadowpass_buffer =
//      create_frame_buffer(shadowpass.get_api_pass(), 1, image_views,
//                          shadowmap_width, shadowmap_height);
//}

void GraphicsImpl::create_output_buffers()
{
	size_t image_num = swapchain.getSwapchainSize();
	output_buffers.resize(image_num);

	for (size_t i = 0; i < image_num; i++)
	{
		vk::ImageView image_views[2] = {
			output_images[i].get_api_image_view(),
			depth_image.get_api_image_view(),
		};

		output_buffers[i] = create_frame_buffer(
			render_pass.get_api_pass(), 2, image_views,
			swapchain.get_extent().width, swapchain.get_extent().height);
	}
}

vk::Framebuffer GraphicsImpl::create_frame_buffer(vk::RenderPass pass,
												  uint32_t attachment_count,
												  vk::ImageView* p_attachments,
												  uint32_t width,
												  uint32_t height)
{
	auto info = vk::FramebufferCreateInfo({}, pass, attachment_count,
										  p_attachments, width, height, 1);

	auto frame_buffer = p_device->get().createFramebuffer(info);

	return frame_buffer;
}

void GraphicsImpl::create_semaphores()
{
// create required sephamores
	VkSemaphoreCreateInfo semaphoreBegin{};
	semaphoreBegin.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(p_device->get(), &semaphoreBegin, nullptr,
							  &image_available_semaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(p_device->get(), &semaphoreBegin, nullptr,
							  &render_finished_semaphores[i]) != VK_SUCCESS)
		{

			throw std::runtime_error("could not create semaphore ready signal");
		}
	}
}

void GraphicsImpl::create_fences()
{
	in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
	images_in_flight.resize(swapchain.getSwapchainSize(), VK_NULL_HANDLE);
	VkFenceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateFence(p_device->get(), &createInfo, nullptr,
						  &in_flight_fences[i]) != VK_SUCCESS)
		{

			throw std::runtime_error("failed to create fences");
		};
	}
}

//void GraphicsImpl::create_shadowpass_resources() {
//  mem::ImageCreateInfo shadow_image_info{};
//  shadow_image_info.extent = VkExtent3D{shadowmap_width, shadowmap_height, 1};
//
//  shadow_image_info.format = vk::Format::eD16Unorm;
//  shadow_image_info.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment |
//                            vk::ImageUsageFlagBits::eSampled;
//  shadow_image_info.queueFamilyIndexCount = 1;
//  shadow_image_info.pQueueFamilyIndices = &p_device->get_graphics_family();
//
//  VkImageViewUsageCreateInfo usageInfo{};
//  usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
//  usageInfo.usage =
//      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
//
//  // setup create struct for image views
//  mem::ImageViewCreateInfo createInfo{};
//  createInfo.pNext = &usageInfo;
//  createInfo.aspect_mask = vk::ImageAspectFlagBits::eDepth;
//
//  mem::ImageData data{};
//  data.image_info = shadow_image_info;
//  data.image_view_info = createInfo;
//
//  shadow_pass_texture.init(p_physical_device, p_device, data);
//}

void GraphicsImpl::create_skybox_pipeline()
{
	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS,
	};

	std::vector<VkPushConstantRange> push_ranges;

	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(glm::vec4);

	push_ranges.push_back(pushRange);

	PipelineConfig config{};
	config.vert_shader_path = SHADER("skybox.vert");
	config.frag_shader_path = SHADER("skybox.frag");
	config.dynamic_states = dynamic_states;
	config.pass = skybox_pass.get_api_pass();
	config.subpass_index = 0;
	config.screen_extent = swapchain.get_extent();
	config.push_ranges = push_ranges;
	config.blend_colours = true;
	config.cull_mode = VK_CULL_MODE_FRONT_BIT;
	config.front_face = VK_FRONT_FACE_CLOCKWISE;

	skybox_pipeline.init(p_device, set_pool, config);

}

void GraphicsImpl::create_graphics_pipeline()
{
// create 2 pipelines, 1 for materials with textures, one without
	graphics_pipelines.resize(2);

	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS,
	};

	// TEX
	//std::vector<VkDescriptorSetLayout> descriptor_layouts = { ubo_layout, texture_layout, matLayout, scene_layout };

	std::vector<VkPushConstantRange> push_ranges;

	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(LightObject) + sizeof(glm::vec4);

	push_ranges.push_back(pushRange);

	PipelineConfig config{};
	config.vert_shader_path = SHADER_PATH + "shader.vert";
	config.frag_shader_path = SHADER_PATH + "shader.frag";
	config.dynamic_states = dynamic_states;
	//config.descriptor_layouts = descriptor_layouts;
	config.push_ranges = push_ranges;
	config.pass = render_pass.get_api_pass();
	config.subpass_index = 0;
	config.screen_extent = swapchain.get_extent();
	config.blend_colours = true;

	graphics_pipelines[0].init(p_device, set_pool, config);

	//auto it = config.descriptor_layouts.begin() + 1;
	//config.descriptor_layouts.erase(it);
	config.frag_shader_path = SHADER_PATH + "no_texture.frag";

	graphics_pipelines[1].init(p_device, set_pool, config);
}

void GraphicsImpl::create_oit_pass()
{
	ColourConfig config{};
	config.format = swapchain.get_format();
	config.final_layout = vk::ImageLayout::eColorAttachmentOptimal;

	oit_pass.add_colour(0, config);
	oit_pass.create_subpass(VK_PIPELINE_BIND_POINT_GRAPHICS, true, false);
	oit_pass.build(*p_device, VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void GraphicsImpl::create_depth_resources()
{
// create image to represent depth
	br::ImageCreateInfo depth_image_info{};
	depth_image_info.extent = VkExtent3D{ swapchain.get_extent().width,
										 swapchain.get_extent().height, 1 };
	depth_image_info.format = vk::Format::eD16Unorm;
	depth_image_info.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	depth_image_info.queueFamilyIndexCount = 1;
	depth_image_info.pQueueFamilyIndices = &p_device->get_graphics_family();

	// create image view
	VkImageViewUsageCreateInfo usageInfo{};
	usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
	usageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	// setup create struct for image views
	br::ImageViewCreateInfo depth_image_view_info{};
	depth_image_view_info.pNext = &usageInfo;
	depth_image_view_info.format = vk::Format::eD16Unorm;

	depth_image_view_info.aspect_mask = vk::ImageAspectFlagBits::eDepth;

	br::ImageData data{};
	data.name = "Depth Image";
	data.image_info = depth_image_info;
	data.image_view_info = depth_image_view_info;

	// create memory for image
	depth_image.init(p_physical_device, p_device, data);
}

//void GraphicsImpl::create_shadowpass() {
//  std::vector<vk::SubpassDependency> dependencies;
//
//  auto dependency_one = vk::SubpassDependency(
//      VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eFragmentShader,
//      vk::PipelineStageFlagBits::eEarlyFragmentTests,
//      vk::AccessFlagBits::eShaderRead,
//      vk::AccessFlagBits::eDepthStencilAttachmentWrite,
//      vk::DependencyFlagBits::eByRegion);
//
//  auto dependency_two = vk::SubpassDependency(
//      0, VK_SUBPASS_EXTERNAL, vk::PipelineStageFlagBits::eLateFragmentTests,
//      vk::PipelineStageFlagBits::eFragmentShader,
//      vk::AccessFlagBits::eDepthStencilAttachmentWrite,
//      vk::AccessFlagBits::eShaderRead, vk::DependencyFlagBits::eByRegion);
//
//  dependencies.push_back(dependency_one);
//  dependencies.push_back(dependency_two);
//
//  DepthConfig config{};
//  config.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//
//  shadowpass.add_depth(0, config);
//  shadowpass.add_dependency(dependencies);
//  shadowpass.create_subpass(VK_PIPELINE_BIND_POINT_GRAPHICS, false, true);
//  shadowpass.build(p_device, VK_PIPELINE_BIND_POINT_GRAPHICS);
//}

void GraphicsImpl::create_geometry_buffer()
{
// create_frame_buffer(&g_buffer);
}

void GraphicsImpl::create_output_images()
{
	br::ImageData data{};
	data.name = "Output Images";
	data.image_info.extent.width = swapchain.get_extent().width;
	data.image_info.extent.height = swapchain.get_extent().height;
	data.image_info.extent.depth = 1.0;

	data.image_info.format = vk::Format::eR32G32B32A32Sfloat;

	data.image_info.usage = vk::ImageUsageFlagBits::eColorAttachment |
		vk::ImageUsageFlagBits::eTransferSrc |
		vk::ImageUsageFlagBits::eSampled;

	data.image_info.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

	data.image_info.queueFamilyIndexCount = 1;
	data.image_info.pQueueFamilyIndices = &p_device->get_graphics_family();

	data.image_view_info.aspect_mask = vk::ImageAspectFlagBits::eColor;

	output_images.resize(swapchain.getSwapchainSize());

	for (br::Image& image : output_images)
	{
		image.init(p_physical_device, p_device, data);
	}
}

// ---- TucoPass ----
void GraphicsImpl::create_render_pass()
{
	ColourConfig config{};
	config.format = vk::Format::eR32G32B32A32Sfloat;
	config.initial_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	config.final_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	config.load_op = vk::AttachmentLoadOp::eLoad;

	render_pass.init(p_device, true, true, config);
}

void GraphicsImpl::create_skybox_pass()
{
	ColourConfig config{};
	config.format = vk::Format::eR32G32B32A32Sfloat;
	config.final_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	config.load_op = vk::AttachmentLoadOp::eClear;
	skybox_pass.init(p_device, true, true, config);
}
// ---- TucoPass ----

void GraphicsImpl::create_light_layout()
{
/* UNIFORM BUFFER DESCRIPTOR SET */
	VkDescriptorSetLayoutBinding ubo_layout_binding{};
	ubo_layout_binding.binding = 1;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	layout_info.bindingCount = 1;
	layout_info.pBindings = &ubo_layout_binding;

	if (vkCreateDescriptorSetLayout(p_device->get(), &layout_info, nullptr,
									&light_layout) != VK_SUCCESS)
	{
		ERR("Failed to create descriptor set.");
	}
}

void GraphicsImpl::create_scene_layout()
{
	// IBL Texture
	VkDescriptorSetLayoutBinding iblTexBinding{};
	iblTexBinding.binding = 0;
	iblTexBinding.descriptorCount = 1;
	iblTexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	iblTexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	iblTexBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = 1;
	VkDescriptorSetLayoutBinding bindings[1] = { iblTexBinding };
	layout_info.pBindings = bindings;

	VkResult result = vkCreateDescriptorSetLayout(p_device->get(), &layout_info, nullptr, &scene_layout);

	ASSERT(result == VK_SUCCESS, "failed to create scene layout");
}

void GraphicsImpl::createMaterialLayout()
{
// Uniform material data binding
	VkDescriptorSetLayoutBinding matLayoutBinding{};
	matLayoutBinding.binding = 0;
	matLayoutBinding.descriptorCount = 1;
	matLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	matLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	matLayoutBinding.pImmutableSamplers = nullptr;

	// Material base color texture binding
	VkDescriptorSetLayoutBinding matBaseTexBinding{};
	matBaseTexBinding.binding = 1;
	matBaseTexBinding.descriptorCount = 1;
	matBaseTexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	matBaseTexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	matBaseTexBinding.pImmutableSamplers = nullptr;

	// Material roughness color texture binding
	VkDescriptorSetLayoutBinding matRoughnessTexBinding{};
	matRoughnessTexBinding.binding = 2;
	matRoughnessTexBinding.descriptorCount = 1;
	matRoughnessTexBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	matRoughnessTexBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	matRoughnessTexBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	layoutInfo.bindingCount = 3;
	VkDescriptorSetLayoutBinding bindings[3] = { matLayoutBinding,
												matBaseTexBinding,
												matRoughnessTexBinding };
	layoutInfo.pBindings = bindings;

	VkResult result = vkCreateDescriptorSetLayout(p_device->get(), &layoutInfo,
												  nullptr, &matLayout);

	if (result != VK_SUCCESS)
	{
		ERR("Could not create materials layout");
	}
}

// PURPOSE: create pool to allocate memory to.
void GraphicsImpl::createMaterialPool()
{
	std::vector<mem::PoolCreateInfo> poolInfo(2);
	// material shader paramaters
	poolInfo[0].pool_size = MATERIALS_POOL_SIZE;
	poolInfo[0].set_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// material diffuse texture
	poolInfo[1].pool_size = MATERIALS_POOL_SIZE;
	poolInfo[1].set_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	matPool = std::make_unique<mem::Pool>(p_device, poolInfo);
}

void GraphicsImpl::create_set_pool()
{

	std::vector<mem::PoolCreateInfo> info;
	v::Limits& limits = v::Limits::get();
	for (const auto type : limits.descriptor_types)
	{
		mem::PoolCreateInfo pool_info{};
		pool_info.pool_size = POOL_MAX_SET_COUNT;
		pool_info.set_type = type;

		info.push_back(pool_info);
	}

	set_pool = std::make_shared<mem::Pool>(p_device, info);
}

void GraphicsImpl::createMaterialCollection()
{
	// matSets.resize(matSets.size() + 1);
	// matSets[matSets.size() - 1].resize(matCount);

	// matPool->allocateDescriptorSets(p_device, matCount, matLayout,
	//                                 matSets[matSets.size() - 1].data());

	materialCollection.init(*p_device, matLayout);
}

void GraphicsImpl::create_scene_collection()
{
	scene_collection.init(*p_device, scene_layout);
}

void GraphicsImpl::create_screen_pipeline()
{
	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS,
	};

	std::vector<VkDescriptorSetLayout> descriptor_layouts;
	descriptor_layouts.push_back(texture_layout);

	PipelineConfig config{};
	config.vert_shader_path = SHADER_PATH + "quad.vert";
	config.frag_shader_path = SHADER_PATH + "quad.frag";
	config.dynamic_states = dynamic_states;
	//config.descriptor_layouts = descriptor_layouts;
	config.pass = screen_pass.get_api_pass();
	config.subpass_index = 0;
	config.screen_extent = swapchain.get_extent();
	config.blend_colours = false;
	config.attribute_descriptions = {};
	config.binding_descriptions = {};
	config.cull_mode = VK_CULL_MODE_FRONT_BIT;
	config.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	screen_pipeline.init(p_device, set_pool, config);
}

void GraphicsImpl::create_screen_pass()
{
	ColourConfig config{};
	config.format = swapchain.get_format();
	config.final_layout = vk::ImageLayout::ePresentSrcKHR;

	std::vector<vk::SubpassDependency> dependencies(2);

	dependencies[0] = vk::SubpassDependency(
		VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
		vk::DependencyFlagBits::eByRegion);

	dependencies[1] = vk::SubpassDependency(
		0, VK_SUBPASS_EXTERNAL, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::AccessFlagBits::eColorAttachmentWrite,
		vk::AccessFlagBits::eMemoryRead, vk::DependencyFlagBits::eByRegion);

	screen_pass.add_colour(0, config);
	screen_pass.create_subpass(VK_PIPELINE_BIND_POINT_GRAPHICS, true, false);
	screen_pass.build(*p_device, VK_PIPELINE_BIND_POINT_GRAPHICS);
}

void GraphicsImpl::create_screen_buffer()
{
	uint32_t image_num = swapchain.getSwapchainSize();
	screen_buffers.resize(image_num);

	for (size_t i = 0; i < image_num; i++)
	{
		std::vector<vk::ImageView> image_views = {
			swapchain.get_image(i).get_api_image_view(),
		};

		screen_buffers[i] = create_frame_buffer(
			screen_pass.get_api_pass(), image_views.size(), image_views.data(),
			swapchain.get_extent().width, swapchain.get_extent().height);
	}
}

void GraphicsImpl::create_ubo_layout()
{
/* UNIFORM BUFFER DESCRIPTOR SET */
	VkDescriptorSetLayoutBinding ubo_layout_binding{};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	layout_info.bindingCount = 1;
	layout_info.pBindings = &ubo_layout_binding;

	auto result = vkCreateDescriptorSetLayout(p_device->get(), &layout_info, nullptr,
											  &ubo_layout);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("could not create ubo layout");
	}
}

void GraphicsImpl::create_ubo_pool()
{
	std::vector<mem::PoolCreateInfo> poolInfo(1);
	poolInfo[0].pool_size = swapchain.getSwapchainSize() * 50;
	poolInfo[0].set_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	ubo_pool = std::make_unique<mem::Pool>(p_device, poolInfo);
}

// create swapchain image * set_count amount of descriptor sets.
void GraphicsImpl::createUboSets(uint32_t setCount)
{
	size_t current_size = uboSets.size();
	uboSets.resize(current_size + 1);
	uboSets[current_size].resize(setCount);

	ubo_pool->allocateDescriptorSets(*p_device, setCount, ubo_layout,
									 uboSets[current_size].data());
}

//void GraphicsImpl::create_shadowmap_pool() {
//  std::vector<mem::PoolCreateInfo> poolInfo(1);
//  poolInfo[0].pool_size = 100;
//  poolInfo[0].set_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//
//  shadowmap_pool = std::make_unique<mem::Pool>(p_device, poolInfo);
//}

void GraphicsImpl::create_texture_pool()
{
	std::vector<mem::PoolCreateInfo> poolInfo(1);
	poolInfo[0].pool_size = swapchain.getSwapchainSize() * 100;
	poolInfo[0].set_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	texture_pool = std::make_unique<mem::Pool>(p_device, poolInfo);
}

//void GraphicsImpl::create_shadowmap_set() {
//  shadowmap_pool->allocateDescriptorSets(p_device, 1, shadowmap_layout,
//                                         &shadowmap_set);
//}

void GraphicsImpl::create_texture_set(size_t mesh_count)
{
	size_t current_size = texture_sets.size();
	texture_sets.resize(current_size + 1);
	// texture_sets[current_size] = create_set(texture_layout, mesh_count,
	// *texture_pool);
	texture_sets[current_size].init(*p_device, texture_layout);

	texture_sets[current_size].addSets(mesh_count, *texture_pool);
}

std::vector<VkDescriptorSet>
GraphicsImpl::create_set(VkDescriptorSetLayout layout, size_t set_count,
						 mem::Pool& pool)
{
	std::vector<VkDescriptorSetLayout> layouts(set_count, layout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = pool.pools[pool.allocate(set_count)];
	allocateInfo.descriptorSetCount = set_count;
	allocateInfo.pSetLayouts = layouts.data();

	std::vector<VkDescriptorSet> sets(set_count);
	VkResult result =
		vkAllocateDescriptorSets(p_device->get(), &allocateInfo, sets.data());

	if (result != VK_SUCCESS)
	{
		printf("[ERROR CODE %d] - could not allocate sets \n", result);
		throw new std::runtime_error("");
	}

	return sets;
}

//void GraphicsImpl::create_shadowmap_layout() {
//  /* SAMPLED IMAGE DESCRIPTOR SET (FOR TEXTURING) */
//  VkDescriptorSetLayoutBinding texture_layout_binding{};
//  texture_layout_binding.binding = 1;
//  texture_layout_binding.descriptorCount = 1;
//  texture_layout_binding.descriptorType =
//      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//  texture_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//  texture_layout_binding.pImmutableSamplers = nullptr;
//
//  VkDescriptorSetLayoutCreateInfo layout_info{};
//  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//
//  layout_info.bindingCount = 1;
//  layout_info.pBindings = &texture_layout_binding;
//
//  if (vkCreateDescriptorSetLayout(p_device->get(), &layout_info, nullptr,
//                                  &shadowmap_layout) != VK_SUCCESS) {
//    throw std::runtime_error("could not create texture layout");
//  }
//}

void GraphicsImpl::create_texture_layout()
{
/* SAMPLED IMAGE DESCRIPTOR SET (FOR TEXTURING) */
	VkDescriptorSetLayoutBinding texture_layout_binding{};
	texture_layout_binding.binding = 0;
	texture_layout_binding.descriptorCount = 1;
	texture_layout_binding.descriptorType =
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	texture_layout_binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	layout_info.bindingCount = 1;
	layout_info.pBindings = &texture_layout_binding;

	if (vkCreateDescriptorSetLayout(p_device->get(), &layout_info, nullptr,
									&texture_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("could not create texture layout");
	}
}

VkShaderModule
GraphicsImpl::create_shader_module(std::vector<uint32_t> shaderCode)
{
	VkShaderModule shaderModule;

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);

	createInfo.pCode = shaderCode.data();

	if (vkCreateShaderModule(p_device->get(), &createInfo, nullptr, &shaderModule) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("could not create shader module");
	}

	return shaderModule;
}

VkPipelineShaderStageCreateInfo
GraphicsImpl::fill_shader_stage_struct(VkShaderStageFlagBits stage,
									   VkShaderModule shaderModule)
{
	VkPipelineShaderStageCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.stage = stage;
	createInfo.module = shaderModule;
	createInfo.pName = "main";

	return createInfo;
}

void GraphicsImpl::create_shadowpass_pipeline()
{
	VkExtent2D shadowmap_extent;
	shadowmap_extent.width = shadowmap_width;
	shadowmap_extent.height = shadowmap_height;

	std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT,
												  VK_DYNAMIC_STATE_SCISSOR,
												  VK_DYNAMIC_STATE_DEPTH_BIAS };

	std::vector<VkDescriptorSetLayout> layouts = { light_layout };

	std::vector<VkPushConstantRange> push_ranges;

	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(LightObject) + sizeof(glm::vec4);

	push_ranges.push_back(pushRange);

	PipelineConfig config{};
	config.vert_shader_path = SHADER_PATH + "shadow.vert";
	config.dynamic_states = dynamic_states;
	config.screen_extent = shadowmap_extent;
	config.pass = shadowpass.get_api_pass();
	config.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
	config.subpass_index = 0;
	config.depth_bias_enable = VK_TRUE;
	config.push_ranges = push_ranges;

	shadowmap_pipeline.init(p_device, set_pool, config);
}

VkDescriptorBufferInfo GraphicsImpl::allocateDescriptorBuffer(uint32_t size)
{
	VkDeviceSize offset = uniform_buffer.allocate(size, v::Limits::get().uniformBufferOffsetAlignment);

	VkDescriptorBufferInfo buffer_info{};
	buffer_info.buffer = uniform_buffer.buffer;
	buffer_info.offset = offset;
	buffer_info.range = (VkDeviceSize)size;

	return buffer_info;
}

void GraphicsImpl::update_descriptor_set(VkDescriptorBufferInfo buffer_info,
										 uint32_t dst_binding,
										 VkDescriptorSet set)
{
	VkWriteDescriptorSet writeInfo{};
	writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeInfo.dstBinding = dst_binding;
	writeInfo.dstSet = set;
	writeInfo.descriptorCount = 1;
	writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeInfo.pBufferInfo = &buffer_info;
	writeInfo.dstArrayElement = 0;

	vkUpdateDescriptorSets(p_device->get(), 1, &writeInfo, 0, nullptr);
}

void GraphicsImpl::write_to_ubo()
{
	for (size_t i = 0; i < uboSets[uboSets.size() - 1].size(); i++)
	{
		VkDescriptorBufferInfo buffer_info =
			allocateDescriptorBuffer(sizeof(UniformBufferObject));
		ubo_offsets.push_back(buffer_info.offset);
		update_descriptor_set(buffer_info, 0, uboSets[uboSets.size() - 1][i]);
	}
}

void GraphicsImpl::destroy_draw()
{
	vkDeviceWaitIdle(p_device->get());

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyFence(p_device->get(), in_flight_fences[i], nullptr);

		vkDestroySemaphore(p_device->get(), render_finished_semaphores[i], nullptr);

		vkDestroySemaphore(p_device->get(), image_available_semaphores[i], nullptr);
	}

	uniform_buffer.destroy();
	vertex_buffer.destroy();
	index_buffer.destroy();

	vkDestroyCommandPool(p_device->get(), command_pool, nullptr);

	vkDestroyDescriptorSetLayout(p_device->get(), ubo_layout, nullptr);
	vkDestroyDescriptorSetLayout(p_device->get(), light_layout, nullptr);
	//vkDestroyDescriptorSetLayout(p_device->get(), shadowmap_layout, nullptr);
	vkDestroyDescriptorSetLayout(p_device->get(), texture_layout, nullptr);
	vkDestroyDescriptorSetLayout(p_device->get(), matLayout, nullptr);
	vkDestroyDescriptorSetLayout(p_device->get(), scene_layout, nullptr);

	vkDestroySampler(p_device->get(), texture_sampler, nullptr);
	//vkDestroySampler(p_device->get(), shadowmap_sampler, nullptr);

	for (auto& graphics_pipeline : graphics_pipelines)
	{
		graphics_pipeline.destroy();
	}

	screen_pipeline.destroy();
	skybox_pipeline.destroy();

	for (const auto& output_buffer : output_buffers)
	{
		vkDestroyFramebuffer(p_device->get(), output_buffer, nullptr);
	}

	for (const auto& screen_buffer : screen_buffers)
	{
		vkDestroyFramebuffer(p_device->get(), screen_buffer, nullptr);
	}
	vkDestroyFramebuffer(p_device->get(), shadowpass_buffer, nullptr);

	texture_pool.get()->destroyPool();
	ubo_pool.get()->destroyPool();
	matPool.get()->destroyPool();
	//shadowmap_pool.get()->destroyPool();

	//for (br::Image &image : output_images) {
	//  image.destroy();
	//}

	//depth_image.destroy();

	//for (size_t i = 0; i < texture_images.size(); i++) {
	//  for (br::Image &image : texture_images[i]) {
	//    image.destroy();
	//  }
	//}

	//shadow_pass_texture.destroy();

	render_pass.destroy();
	skybox_pass.destroy();
	screen_pass.destroy();
	//shadowpass.destroy();
}

vk::SurfaceFormatKHR
choose_best_surface_format(std::vector<vk::SurfaceFormatKHR> formats)
{
// from here we can probably choose the best format, though i don't really
// remember what the best formats looked like
	for (const auto& format : formats)
	{
		if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear &&
			format.format == vk::Format::eR8G8B8A8Srgb)
		{
			return format;
		}
	}

	// if we couldn't find the best, then we'll make due with whatever is
	// supported
	return formats[0];
}

void GraphicsImpl::create_light_set(uint32_t set_count)
{
// Cannot destroy layout inside ResourceCollection because layout may be shared by multiple different ResourceCollections
// In the below example, when resize event occurs the vector needs to copy all the elements to a larger buffer causing the destruct event.

	uint32_t current_size = light_ubo.size();
	light_ubo.resize(current_size + 1);

	light_ubo[current_size].init(*p_device, light_layout);

	light_ubo[current_size].addSets(set_count, *ubo_pool);

	// write to set
	VkDeviceSize offset = uniform_buffer.allocate(sizeof(UniformBufferObject));

	VkDeviceSize buffer_range = (VkDeviceSize)sizeof(UniformBufferObject);
	light_ubo[current_size].addBuffer(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
									  uniform_buffer.buffer, offset,
									  buffer_range);

	light_ubo[current_size].updateSets();
	for (uint32_t i = 0; i < set_count; i++)
	{
		light_offsets.push_back(offset);
	}
}

void GraphicsImpl::free_command_buffers()
{
	if (command_buffers.size() == 0)
		return;

	vkWaitForFences(p_device->get(), 1, &in_flight_fences[submitted_frame], VK_TRUE,
					UINT64_MAX);

	p_device->get().freeCommandBuffers(command_pool,
									   static_cast<uint32_t>(command_buffers.size()),
									   command_buffers.data());
}

void GraphicsImpl::create_shadow_map(
	const std::vector<std::unique_ptr<GameObject>>& game_objects,
	size_t command_index, LightObject light)
{
	VkRenderPassBeginInfo shadowpass_info{};

	shadowpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	shadowpass_info.renderPass = shadowpass.get_api_pass();
	shadowpass_info.framebuffer = shadowpass_buffer;
	VkRect2D renderArea{};
	renderArea.offset = VkOffset2D{ 0, 0 };
	renderArea.extent = VkExtent2D{ shadowmap_width, shadowmap_height };
	shadowpass_info.renderArea = renderArea;

	VkClearValue shadowpass_clear;
	shadowpass_clear.depthStencil = { 1.0, 0 };

	shadowpass_info.clearValueCount = 1;
	shadowpass_info.pClearValues = &shadowpass_clear;

	// NOTE: when imeplementing multiple shadowmaps, we can loop through
	// MAX_SHADOW_CASTERS twice. for (size_t y = 0; y <
	// glm::sqrt(MAX_SHADOW_CASTERS); y++) {
	//     for (size_t x = 0; x < glm::sqrt(MAX_SHADOW_CASTERS); x++) {
	// there is at least one light that is casting a shadow here
	vkCmdBeginRenderPass(command_buffers[command_index], &shadowpass_info,
						 VK_SUBPASS_CONTENTS_INLINE);

	VkViewport shadowpass_port = {};
	shadowpass_port.x = 0;
	shadowpass_port.y = 0;
	shadowpass_port.width = (float)shadowmap_width;
	shadowpass_port.height = (float)shadowmap_height;
	shadowpass_port.minDepth = 0.0;
	shadowpass_port.maxDepth = 1.0;
	vkCmdSetViewport(command_buffers[command_index], 0, 1, &shadowpass_port);

	VkRect2D shadowpass_scissor{};
	shadowpass_scissor.offset = { 0, 0 };
	shadowpass_scissor.extent.width = shadowmap_width;
	shadowpass_scissor.extent.height = shadowmap_height;
	vkCmdSetScissor(command_buffers[command_index], 0, 1, &shadowpass_scissor);

	vkCmdSetDepthBias(command_buffers[command_index], depth_bias_constant, 0.0f,
					  depth_bias_slope);

	vkCmdBindPipeline(command_buffers[command_index],
					  VK_PIPELINE_BIND_POINT_GRAPHICS,
					  shadowmap_pipeline.get_api_pipeline());

	// time for the draw calls
	const VkDeviceSize offsets[] = { 0, offsetof(Vertex, normal),
									offsetof(Vertex, tex_coord) };

	command_buffers[command_index].bindVertexBuffers(0, 1, &vertex_buffer.buffer,
													 offsets);

	vkCmdBindIndexBuffer(command_buffers[command_index], index_buffer.buffer, 0,
						 VK_INDEX_TYPE_UINT32);

	vkCmdPushConstants(command_buffers[command_index],
					   shadowmap_pipeline.get_api_layout(),
					   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(light), &light);

	for (size_t j = 0; j < game_objects.size(); j++)
	{
		if (!game_objects[j]->update)
		{
			for (size_t k = 0; k < game_objects[j]->object_model.primitives.size();
				 k++)
			{
				Primitive prim = game_objects[j]->object_model.primitives[k];

				VkDescriptorSet descriptors[1] = {
					light_ubo[j].get_api_set(prim.transform_index) };

				vkCmdBindDescriptorSets(
					command_buffers[command_index], VK_PIPELINE_BIND_POINT_GRAPHICS,
					shadowmap_pipeline.get_api_layout(), 0, 1, descriptors, 0, nullptr);

				vkCmdDrawIndexed(command_buffers[command_index], prim.index_count, 1,
								 prim.index_start, 0, static_cast<uint32_t>(0));
			}
		}
	}

	vkCmdEndRenderPass(command_buffers[command_index]);
}

void GraphicsImpl::updateMaterialResources(Material& material)
{
// update_materials(material.gpuInfo.descriptorOffset, material);
}

void GraphicsImpl::writeSceneCollection(SceneData& scene)
{

}

void GraphicsImpl::create_scene(SceneData* scene) {
	ResourceCollection* skybox_collection = skybox_pipeline.get_resource_collection(0);
	uint32_t index = skybox_collection->addSets(1, *set_pool);
	scene->set_index(skybox_collection, index);

	//ResourceCollection* forward_collection = graphics_pipelines[0].get_resource_collection(3);
	//scene->set_index(forward_collection, forward_collection->addSets(1, *set_pool));
}

void GraphicsImpl::write_scene(SceneData* scene)
{
	GameObject& skybox_model = scene->get_skybox_model();
	
	scene->ubo_offset = uniform_buffer.allocate(sizeof(UniformBufferObject), v::Limits::get().uniformBufferOffsetAlignment);

	BufferDescription info{};
	info.binding = 1;
	info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	info.buffer = uniform_buffer.buffer;
	info.bufferRange = sizeof(UniformBufferObject);
	info.bufferOffset = scene->ubo_offset;

	ResourceCollection* skybox_collection = skybox_pipeline.get_resource_collection(0);
	skybox_collection->addBuffer(info, scene->get_index(skybox_collection));

	// Skybox texture
	ImageDescription image_info{};
	image_info.binding = 0;
	image_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	image_info.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// TODO: if scene does not have skybox, disable pass.
	if (scene->has_skybox)
	{
		image_info.image = scene->get_skybox().get_image().get_api_image();
		image_info.image_view = scene->get_skybox().get_image().get_api_image_view();
		image_info.sampler = scene->get_skybox().get_image().get_sampler();
	}

	skybox_collection->addImage(image_info, scene->get_index(skybox_collection));

	//ResourceCollection* forward_collection = graphics_pipelines[0].get_resource_collection(3);
	//// IBL texture
	//ImageDescription image_info{};
	//image_info.binding = 0;
	//image_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//image_info.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//if (scene->has_ibl)
	//{
	//	image_info.image = scene->get_ibl().get_api_image();
	//	image_info.image_view = scene->get_ibl().get_api_image_view();
	//	image_info.sampler = scene->get_ibl().get_sampler();
	//}
	//forward_collection->addImage(image_info, scene->get_index(forward_collection));

	//forward_collection->updateSet(scene->get_index(forward_collection));
	skybox_collection->updateSet(scene->get_index(skybox_collection));
}

void GraphicsImpl::writeMaterial(Material& material)
{
	// material.offsets.descriptorOffset = globalMaterialOffsets.descriptorOffset;
	material.gpuInfo.bufferOffset =
		uniform_buffer.allocate(sizeof(MaterialBufferObject), v::Limits::get().uniformBufferOffsetAlignment);
	// update_materials(material.offsets.bufferOffset, material);
	MaterialBufferObject matObj = material.convert();
	updateUniformBuffer(material.gpuInfo.bufferOffset,
						sizeof(MaterialBufferObject), &matObj);


	// [TODO 08/2024] - by having addSets here, we basically create a new descriptor set every frame per every object... this is memory leak.
	//material.gpuInfo.setIndex = materialCollection.addSets(1, *matPool);
	ResourceCollection* collection = graphics_pipelines[1].get_resource_collection(1);
	material.gpuInfo.setIndex = collection->addSets(1, *matPool);

	BufferDescription info{};
	info.binding = 0;
	info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	info.buffer = uniform_buffer.buffer;
	info.bufferRange = sizeof(MaterialBufferObject);
	info.bufferOffset = material.gpuInfo.bufferOffset;

	collection->addBuffer(info, material.gpuInfo.setIndex);

	// TODO : will cause pipeline warning (errors?) if we do not bind an image even for materials that do not access it.
	//        use specialization constants to prevent compilation of shaders with unbound textures?
	ImageDescription image_info{};
	image_info.binding = 1;
	image_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	image_info.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if (material.hasBaseTexture)
	{
		image_info.image = material.getBaseColorImage().get_api_image();
		image_info.image_view = material.getBaseColorImage().get_api_image_view();
		image_info.sampler = material.getBaseColorImage().get_sampler();
	}
	else
	{
		uninitalized_image.change_layout(vk::ImageLayout::eShaderReadOnlyOptimal, p_device->get_transfer_queue());
		image_info.image = uninitalized_image.get_api_image();
		image_info.image_view = uninitalized_image.get_api_image_view();
		image_info.sampler = uninitalized_image.get_sampler();
	}
	collection->addImage(image_info, material.gpuInfo.setIndex);

	image_info.binding = 2;
	image_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	image_info.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (material.hasRoughnessTexture || material.hasMetallicTexture)
	{
		image_info.image = material.getRoughnessMetallicImage().get_api_image();
		image_info.image_view = material.getRoughnessMetallicImage().get_api_image_view();
		image_info.sampler = material.getRoughnessMetallicImage().get_sampler();
	}
	else
	{
		uninitalized_image.change_layout(vk::ImageLayout::eShaderReadOnlyOptimal, p_device->get_transfer_queue());
		image_info.image = uninitalized_image.get_api_image();
		image_info.image_view = uninitalized_image.get_api_image_view();
		image_info.sampler = uninitalized_image.get_sampler();
	}
	collection->addImage(image_info, material.gpuInfo.setIndex);

	collection->updateSet(material.gpuInfo.setIndex);
}

uint32_t GraphicsImpl::add_material()
{
	materials.push_back(Material{});
	return materials.size() - 1;
}

uint32_t GraphicsImpl::add_draw_data(br::GPUResource* resource)
{
	std::unique_ptr<br::GPUResource> gpu_resource(resource);
	draw_data.push_back(std::move(gpu_resource));
	return draw_data.size() - 1;
}

void GraphicsImpl::update_draw_item(uint32_t object_index, GameObject& object)
{
	// check if object has associated draw item
	if (draw_items.size() >= object_index)
	{
		draw_items.resize(object_index + 1);

		draw_items[object_index].set_vertex_buffer(&vertex_buffer);
		draw_items[object_index].set_index_buffer(&index_buffer);

		draw_items[object_index].set_pso(&graphics_pipelines[1]);
	}

	br::DrawItem& item = draw_items[object_index];
	uint32_t index_mem = update_index_buffer(object.object_model.model_indices);
	uint32_t vertex_mem = update_vertex_buffer(object.object_model.model_vertices);

	item.set_draw_offsets(index_mem, vertex_mem);

	// set material and draw index
	item.set_material_index(object.material_index);
	item.set_draw_index(object.draw_index);
}

// Left to do
//   - fix any bugs
//   - figure out some way to update MeshDrawData with correct mvp data.
//   - then call item.U

void GraphicsImpl::create_command_buffers(
	const std::vector<std::unique_ptr<GameObject>>& game_objects, SceneData* scene)
{
// allocate memory for command buffer, you have to create a draw command for
// each image
	VkCommandBufferAllocateInfo bufferAllocate{};
	bufferAllocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	bufferAllocate.commandPool = command_pool;
	bufferAllocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	uint32_t imageCount = static_cast<uint32_t>(swapchain.getSwapchainSize());
	bufferAllocate.commandBufferCount = imageCount;

	command_buffers = p_device->get().allocateCommandBuffers(bufferAllocate);

	for (size_t i = 0; i < command_buffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;                  // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		auto begin_info = vk::CommandBufferBeginInfo(beginInfo);
		command_buffers[i].begin(begin_info);

		LightObject light{};
		light.position = light_data[0].position;
		light.direction = light_data[0].target;
		light.color = light_data[0].color;
		light.light_count = glm::vec4(MAX_SHADOW_CASTERS, 1, 1, 1);

		// TODO : support shadow maps.
		//create_shadow_map(game_objects, i, light);

		auto render_area = vk::Rect2D(vk::Offset2D(0, 0), swapchain.get_extent());

		std::vector<VkClearValue> clear_values;

		VkClearValue color_clear{};
		color_clear.color.float32[0] = 0.f;
		color_clear.color.float32[1] = 0.f;
		color_clear.color.float32[2] = 0.f;
		color_clear.color.float32[3] = 0.f;

		VkClearValue depth_clear{};
		depth_clear.depthStencil.depth = 1.f;
		depth_clear.depthStencil.stencil = 0;

		clear_values.push_back(color_clear);
		clear_values.push_back(depth_clear);

		VkRenderPassBeginInfo skybox_info{};
		skybox_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		skybox_info.framebuffer = output_buffers[i];
		skybox_info.renderArea = render_area;
		skybox_info.renderPass = skybox_pass.get_api_pass();
		skybox_info.clearValueCount = clear_values.size();
		skybox_info.pClearValues = clear_values.data();

		command_buffers[i].beginRenderPass(skybox_info, vk::SubpassContents::eInline);

		VkViewport newViewport{};
		newViewport.x = 0;
		newViewport.y = 0;
		newViewport.width = (float)swapchain.get_extent().width;
		newViewport.height = (float)swapchain.get_extent().height;
		newViewport.minDepth = 0.0;
		newViewport.maxDepth = 1.0;

		vkCmdSetViewport(command_buffers[i], 0, 1, &newViewport);
		auto scissor = vk::Rect2D(vk::Offset2D(0, 0), swapchain.get_extent());

		command_buffers[i].setScissor(0, 1, &scissor);

		const VkDeviceSize offset[] = { 0, offsetof(Vertex, normal),
									   offsetof(Vertex, tex_coord) };

		command_buffers[i].bindVertexBuffers(0, 1, &vertex_buffer.buffer, offset);

		vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffer, 0,
			VK_INDEX_TYPE_UINT32);

		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline.get_api_pipeline());

		GameObject& skybox = scene->get_skybox_model();
		ResourceCollection* skybox_scene_collection = skybox_pipeline.get_resource_collection(0);
		VkDescriptorSet skybox_scene_set = skybox_scene_collection->get_api_set(Antuco::get_engine().get_scene()->get_index(skybox_scene_collection));

		vkCmdPushConstants(command_buffers[i],
						   skybox_pipeline.get_api_layout(),
						   VK_SHADER_STAGE_VERTEX_BIT, 0,
						   sizeof(glm::vec4), &camera_pos);

		for (size_t k = 0; k < skybox.object_model.primitives.size(); k++)
		{

			Primitive prim = skybox.object_model.primitives[k];

			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_pipeline.get_api_layout(), 0, 1, &skybox_scene_set, 0, nullptr);

			vkCmdDrawIndexed(
				command_buffers[i], prim.index_count, 1,
				prim.index_start + skybox.buffer_index_offset,
				skybox.buffer_vertex_offset, // indices refer to all vertices in model, then no
							   // vertex offsets are required.
				static_cast<uint32_t>(0));
		}

		vkCmdEndRenderPass(command_buffers[i]);

		VkRenderPassBeginInfo forward_info{};
		forward_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		forward_info.framebuffer = output_buffers[i];
		forward_info.renderArea = render_area;
		forward_info.renderPass = render_pass.get_api_pass();
		forward_info.clearValueCount = clear_values.size();
		forward_info.pClearValues = clear_values.data();

		command_buffers[i].beginRenderPass(forward_info,
										   vk::SubpassContents::eInline);


		vkCmdSetViewport(command_buffers[i], 0, 1, &newViewport);

		command_buffers[i].setScissor(0, 1, &scissor);

		// time for the draw calls
		command_buffers[i].bindVertexBuffers(0, 1, &vertex_buffer.buffer, offset);

		vkCmdBindIndexBuffer(command_buffers[i], index_buffer.buffer, 0,
							 VK_INDEX_TYPE_UINT32);

		auto index = 1;
		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
						  graphics_pipelines[index].get_api_pipeline());

		vkCmdPushConstants(command_buffers[i],
						   graphics_pipelines[index].get_api_layout(),
						   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(light), &light);

		vkCmdPushConstants(command_buffers[i],
						   graphics_pipelines[index].get_api_layout(),
						   VK_SHADER_STAGE_VERTEX_BIT, sizeof(light),
						   sizeof(glm::vec4), &camera_pos);

		// draw first object (cube)
		auto index_offset = uint32_t(0);
		auto vertex_offset = uint32_t(0);

		std::vector<TransparentMesh> transparent_meshes;

		for (size_t j = 0; j < game_objects.size(); j++)
		{
// auto total_vertices = uint32_t(0);
// i actually think this update this is ill-thought out...
			if (!game_objects[j]->update)
			{
				auto model_primitive_count =
					game_objects[j]->object_model.primitives.size();

				vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
								  graphics_pipelines[index].get_api_pipeline());

				Material& mat = game_objects[j]->get_material();
				VkDescriptorSet materialSet =
					graphics_pipelines[1].get_resource_collection(1)->get_api_set(mat.gpuInfo.setIndex);

				for (size_t k = 0; k < model_primitive_count; k++)
				{

					Primitive prim = game_objects[j]->object_model.primitives[k];

					auto descriptor_1 = std::vector<VkDescriptorSet>();
					auto descriptor_2 = std::vector<VkDescriptorSet>();
					auto descriptors = std::vector<std::vector<VkDescriptorSet>>();

					descriptor_1.resize(3);
					descriptor_2.resize(2);

					//descriptor_1[0] = light_ubo[j].get_api_set(prim.transform_index);
					descriptor_1[1] = uboSets[j][prim.transform_index];
					//descriptor_1[3] = shadowmap_set;
					// TODO: currently we update all our descriptor sets once at the
					// beginning of the frame. however, if we want to have per-material
					// descriptors, then we need to update more frequently (whenever the
					// material changes).
					descriptor_1[2] = materialSet;

					descriptor_2[0] = descriptor_1[1];
					descriptor_2[1] = descriptor_1[2];
					//descriptor_2[2] = scene_collection->get_api_set(Antuco::get_engine().get_scene()->get_index(scene_collection));

					auto layout = graphics_pipelines[index].get_api_layout();

					descriptors.push_back(descriptor_1);
					descriptors.push_back(descriptor_2);

					vkCmdBindDescriptorSets(
						command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0,
						descriptors[index].size(), descriptors[index].data(), 0, nullptr);

					vkCmdDrawIndexed(
						command_buffers[i], prim.index_count, 1,
						prim.index_start + game_objects[j]->buffer_index_offset,
						game_objects[j]->buffer_vertex_offset, // indices refer to all vertices in model, then no
									   // vertex offsets are required.
						static_cast<uint32_t>(0));
				}
			}

			index_offset += static_cast<uint32_t>(
				game_objects[j]->object_model.model_indices.size());
			vertex_offset += static_cast<uint32_t>(
				game_objects[j]->object_model.model_vertices.size());
		}
		vkCmdEndRenderPass(command_buffers[i]);

		// copy_to_swapchain(i);
		render_to_screen(i);

		// switch image back to depth stencil layout for the next render pa
		// end commands to go to execute stage
		if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("could not end command buffer");
		}
	}
}

// creates memory dependency which ensures that the data in some is properly
// written to before being read.
void GraphicsImpl::memory_dependency(size_t i, VkAccessFlags src_a,
									 VkAccessFlags dst_a,
									 VkPipelineStageFlags src_p,
									 VkPipelineStageFlags dst_p)
{
	VkMemoryBarrier mem_barrier{};
	mem_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	mem_barrier.pNext = nullptr;
	mem_barrier.srcAccessMask = src_a;
	mem_barrier.dstAccessMask = dst_a;

	// initiate pipeline barrier
	vkCmdPipelineBarrier(command_buffers[i], src_p, dst_p,
						 VK_DEPENDENCY_BY_REGION_BIT, 1, &mem_barrier, 0, nullptr,
						 0, nullptr);
}

void GraphicsImpl::copy_to_swapchain(size_t i)
{
	swapchain.get_image(i).transfer(vk::ImageLayout::eTransferDstOptimal,
									p_device->get_graphics_queue(),
									command_buffers[i]);

	VkImageSubresourceLayers src_res{};
	VkOffset3D src_offset{};
	VkImageSubresourceLayers dst_res{};
	VkOffset3D dst_offset{};

	VkExtent3D extent = { swapchain.get_extent().width,
						 swapchain.get_extent().height, 1 };

	src_res.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	src_res.baseArrayLayer = 0;
	src_res.layerCount = 1;
	src_res.mipLevel = 0;

	src_offset.x = 0;
	src_offset.y = 0;
	src_offset.z = 0;

	dst_res.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	dst_res.baseArrayLayer = 0;
	dst_res.layerCount = 1;
	dst_res.mipLevel = 0;

	dst_offset.x = 0;
	dst_offset.y = 0;
	dst_offset.z = 0;

	VkImageCopy copy_info{};
	copy_info.srcSubresource = src_res;
	copy_info.dstSubresource = dst_res;
	copy_info.srcOffset = src_offset;
	copy_info.dstOffset = dst_offset;
	copy_info.extent = extent;

	vkCmdCopyImage(command_buffers[i], output_images[i].get_api_image(),
				   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				   swapchain.get_image(i).get_api_image(),
				   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_info);

	swapchain.get_image(i).transfer(vk::ImageLayout::ePresentSrcKHR,
									p_device->get_graphics_queue(),
									command_buffers[i]);
}

void GraphicsImpl::create_screen_set()
{
	screen_resource.init(*p_device, texture_layout);

	screen_resource.addSets(swapchain.getSwapchainSize(), *texture_pool);

	screen_resource.addImagePerSet(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
								   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								   output_images, texture_sampler);

	screen_resource.updateSets();
}

void GraphicsImpl::render_to_screen(size_t i)
{
	auto render_area = vk::Rect2D(vk::Offset2D(0, 0), swapchain.get_extent());

	std::vector<vk::ClearValue> clear_values;
	auto color_clear = vk::ClearValue(
		vk::ClearColorValue(std::array<float, 4>({ 0.f, 0.f, 0.f, 1.f })));
	clear_values.push_back(color_clear);

	auto render_info = vk::RenderPassBeginInfo(
		screen_pass.get_api_pass(), screen_buffers[i], render_area,
		static_cast<uint32_t>(clear_values.size()), clear_values.data());

	command_buffers[i].beginRenderPass(render_info, vk::SubpassContents::eInline);

	// bind pipeline
	vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
					  screen_pipeline.get_api_pipeline());

	VkDescriptorSet descriptors[1] = {
		screen_resource.get_api_set(0),
	};
	// bind descriptor set
	vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
							screen_pipeline.get_api_layout(), 0, 1, descriptors,
							0, nullptr);

	// render 3 vertices to vertex shader
	vkCmdDraw(command_buffers[i], 3, 1, 0, 0);

	vkCmdEndRenderPass(command_buffers[i]);
}

void GraphicsImpl::create_uniform_buffer()
{
	mem::BufferCreateInfo buffer_info{};
	buffer_info.size = (VkDeviceSize)BUFFER_SIZE;
	buffer_info.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	buffer_info.sharing_mode = vk::SharingMode::eExclusive;
	buffer_info.queue_family_index_count = 1;
	buffer_info.p_queue_family_indices = &p_device->get_graphics_family();
	buffer_info.memory_properties = vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent;

	uniform_buffer.init(*p_physical_device, *p_device, buffer_info);
}

void GraphicsImpl::create_vertex_buffer()
{
	mem::BufferCreateInfo buffer_info{};
	buffer_info.size = (VkDeviceSize)BUFFER_SIZE;
	buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer |
		vk::BufferUsageFlagBits::eTransferDst;
	buffer_info.sharing_mode = vk::SharingMode::eExclusive;
	buffer_info.queue_family_index_count = 1;
	buffer_info.p_queue_family_indices = &p_device->get_graphics_family();
	buffer_info.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

	vertex_buffer.init(*p_physical_device, *p_device, buffer_info);
}

void GraphicsImpl::create_index_buffer()
{
	mem::BufferCreateInfo buffer_info{};
	buffer_info.size = (VkDeviceSize)BUFFER_SIZE;
	buffer_info.usage = vk::BufferUsageFlagBits::eIndexBuffer |
		vk::BufferUsageFlagBits::eTransferDst;
	buffer_info.sharing_mode = vk::SharingMode::eExclusive;
	buffer_info.queue_family_index_count = 1;
	buffer_info.p_queue_family_indices = &p_device->get_graphics_family();
	buffer_info.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

	index_buffer.init(*p_physical_device, *p_device, buffer_info);
}

// REQUIRES: indices_data is a list of indices corresponding
//           to the vertices provided to the vertex buffer
// MODIFIES: this
// EFFECTS: allocates memory and maps indices_data to index buffer,
//          returns location in memory as int or '-1' if
//          new data was mapped.
int32_t GraphicsImpl::update_index_buffer(std::vector<uint32_t> indices_data)
{
	if (check_data(indices_data.size() * sizeof(uint32_t)))
	{
		return -1;
	}
	uint32_t mem_location = index_buffer.map(
		indices_data.size() * sizeof(uint32_t), indices_data.data());

	return mem_location;
}

// helper function to check if data exists or print warning if it doesnt
bool GraphicsImpl::check_data(size_t data_size)
{
// do not add data if its size is zero
	if (data_size == 0)
	{
		return true;
	}

	return false;
}

// MODIFIES: this
// PURPOSE: adds given vertex_data to vertex buffer
// RETURNS: memory location of where data was added or '-1' if no new data was
// mapped
int32_t GraphicsImpl::update_vertex_buffer(std::vector<Vertex> vertex_data)
{
	if (check_data(vertex_data.size() * sizeof(Vertex)))
		return -1;

	uint32_t mem_location = vertex_buffer.map(vertex_data.size() * sizeof(Vertex),
											  vertex_data.data());
	return mem_location;
}

void GraphicsImpl::copy_buffer(mem::Memory src_buffer, mem::Memory dst_buffer,
							   VkDeviceSize dst_offset,
							   VkDeviceSize data_size)
{
	auto transfer_buffer = begin_command_buffer(*p_device, command_pool);

	// transfer between buffers
	VkBufferCopy copyData{};
	copyData.srcOffset = 0;
	copyData.dstOffset = dst_offset;
	copyData.size = data_size;

	vkCmdCopyBuffer(transfer_buffer, src_buffer.buffer, dst_buffer.buffer, 1,
					&copyData);

	// destroy transfer buffer, shouldnt need it after copying the data.
	end_command_buffer(*p_device, p_device->get_graphics_queue(), command_pool,
					   transfer_buffer);
}

void GraphicsImpl::update_light_buffer(VkDeviceSize memory_offset,
									   LightBufferObject lbo)
{
	uniform_buffer.writeLocal(p_device->get(), memory_offset, sizeof(lbo), &lbo);
}

void GraphicsImpl::updateUniformBuffer(VkDeviceSize offset, VkDeviceSize size,
									   void* data)
{
	uniform_buffer.writeLocal(p_device->get(), offset, size, data);
}

void GraphicsImpl::update_materials(VkDeviceSize memory_offset, Material mat)
{
	MaterialBufferObject mat_obj = mat.convert();

	uniform_buffer.writeLocal(p_device->get(), memory_offset, sizeof(mat_obj),
							  &mat_obj);
}

void GraphicsImpl::update_uniform_buffer(VkDeviceSize memory_offset,
										 UniformBufferObject ubo)
{
// overwrite memory
	uniform_buffer.writeLocal(p_device->get(), memory_offset, sizeof(ubo), &ubo);
}

/// <summary>
/// When called, this function will destroy all objects associated with the
/// swapchain.
/// </summary>
void GraphicsImpl::cleanup_swapchain()
{
	vkDeviceWaitIdle(p_device->get());

	for (const auto& output_buffer : output_buffers)
	{
		vkDestroyFramebuffer(p_device->get(), output_buffer, nullptr);
	}

	for (const auto& screen_buffer : screen_buffers)
	{
		vkDestroyFramebuffer(p_device->get(), screen_buffer, nullptr);
	}
	vkDestroyFramebuffer(p_device->get(), shadowpass_buffer, nullptr);

	depth_image.destroy();
	render_pass.destroy();
	screen_pass.destroy();

	swapchain.destroy();
}

/// <summary>
/// When called, this function create all necesarry objects for the swapchain
/// </summary>
void GraphicsImpl::recreate_swapchain()
{
	swapchain.init(p_physical_device, p_device, p_surface.get());
	create_depth_resources();
	create_render_pass();
	create_screen_pass();
	create_output_buffers();
	create_screen_buffer();

	// set command buffers to be updated next frame
	update_command_buffers = true;
}

// TODO: need to make better use of cpu-cores
void GraphicsImpl::draw_frame()
{
// make sure that the current frame thats being drawn in parallel is available
	vkWaitForFences(p_device->get(), 1, &in_flight_fences[current_frame], VK_TRUE,
					UINT64_MAX);
	// allocate memory to store next image
	uint32_t nextImage;

	VkResult result = vkAcquireNextImageKHR(
		p_device->get(), swapchain.get(), UINT64_MAX,
		image_available_semaphores[current_frame], VK_NULL_HANDLE, &nextImage);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("could not aquire image from swapchain");
	}

	if (images_in_flight[nextImage] != VK_NULL_HANDLE)
	{
		vkWaitForFences(p_device->get(), 1, &images_in_flight[nextImage], VK_TRUE,
						UINT64_MAX);
		// imagesInFlight[nextImage] = VK_NULL_HANDLE;
	}
	// Mark the image as now being in use by this frame
	images_in_flight[nextImage] = in_flight_fences[current_frame];

	// add appropriate command buffer
	auto wait_stages = std::array<vk::PipelineStageFlags, 1>(
		{ vk::PipelineStageFlagBits::eColorAttachmentOutput });
	auto wait_semaphores =
		std::array<vk::Semaphore, 1>({ image_available_semaphores[current_frame] });
	auto signal_semaphores =
		std::array<vk::Semaphore, 1>({ render_finished_semaphores[current_frame] });

	auto submit_info =
		vk::SubmitInfo(1, wait_semaphores.data(), wait_stages.data(), 1,
					   &command_buffers[nextImage], 1, signal_semaphores.data());

	vkResetFences(p_device->get(), 1, &in_flight_fences[current_frame]);

	p_device->get_graphics_queue().submit(submit_info,
										  in_flight_fences[current_frame]);

	auto pi = vk::PresentInfoKHR(1, signal_semaphores.data(), 1, &swapchain.get(),
								 &nextImage);

	auto present_result = p_device->get_present_queue().presentKHR(pi);

	if (present_result == vk::Result::eErrorOutOfDateKHR ||
		present_result == vk::Result::eSuboptimalKHR)
	{
		cleanup_swapchain();
		recreate_swapchain();
	}

	submitted_frame = current_frame;
	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

//void GraphicsImpl::write_to_shadowmap_set() {
//  VkDescriptorImageInfo imageInfo;
//  imageInfo.sampler = shadowmap_sampler;
//  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//  imageInfo.imageView = shadow_pass_texture.get_api_image_view();
//
//  VkWriteDescriptorSet writeInfo{};
//  writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//  writeInfo.dstBinding = 1;
//  writeInfo.dstSet = shadowmap_set;
//  writeInfo.descriptorCount = 1;
//  writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//  writeInfo.pImageInfo = &imageInfo;
//  writeInfo.dstArrayElement = 0;
//
//  vkUpdateDescriptorSets(p_device->get(), 1, &writeInfo, 0, nullptr);
//
//  // wonder if this is a fine time to transfer the image from undefined to
//  // shader
//  // TODO: move this somewhere else (into its own function if you absolutely
//  // have to) shadowmap_atlas.transfer(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//  // graphics_queue, command_pool);
//}

void GraphicsImpl::write_to_texture_set(ResourceCollection texture_set,
										br::Image image)
{

	texture_set.addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image,
						 texture_sampler);

	texture_set.updateSets();
}

void GraphicsImpl::create_default_images()
{
	// CREATE A WHITE IMAGE TO SEND TO THE SHADER THIS WAY WE CAN ALSO ELIMINATE
	// THE IF STATEMENTS
	br::ImageCreateInfo imageInfo{};
	imageInfo.format = vk::Format::eR8G8B8A8Srgb;
	VkExtent3D extent{};
	extent.width = 1;
	extent.height = 1;
	extent.depth = 1; // this depth might be key to blending the textures...
	imageInfo.extent = extent;
	imageInfo.usage =
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
	imageInfo.queueFamilyIndexCount = 1;
	imageInfo.pQueueFamilyIndices = &p_device->get_graphics_family();
	imageInfo.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

	br::ImageViewCreateInfo viewInfo{};
	viewInfo.aspect_mask = vk::ImageAspectFlagBits::eColor;

	br::ImageData data{};
	data.name = "Uninitialized Texture";
	data.image_info = imageInfo;
	data.image_view_info = viewInfo;

	uninitalized_image.init(p_physical_device, p_device, data);
	uninitalized_image.set_image_sampler(VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

/*
void GraphicsImpl::create_vulkan_image(const ImageBuffer &image, size_t i,
									   size_t j) {
  // create image
  mem::ImageData data{};
  mem::ImageCreateInfo create{};
  mem::ImageViewCreateInfo view{};

  create.format = vk::Format::eR8G8B8A8Srgb;
  create.extent = vk::Extent3D(image.width, image.height, 1);
  create.usage =
	  vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
  create.queueFamilyIndexCount = 1;
  create.pQueueFamilyIndices = &p_device->get_graphics_family();
  create.size = image.buffer_size;
  create.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

  view.aspect_mask = vk::ImageAspectFlagBits::eColor;

  data.image_view_info = view;
  data.image_info = create;

  texture_images.resize(texture_images.size() + 1);
  texture_images[texture_images.size() - 1].resize(1);

  texture_images[texture_images.size() - 1][0].init(p_physical_device, p_device,
													data);

  // create cpu buffer
  mem::BufferCreateInfo texture_buffer_info{};
  texture_buffer_info.size = image.buffer_size;
  texture_buffer_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
  texture_buffer_info.sharing_mode = vk::SharingMode::eExclusive;
  texture_buffer_info.queue_family_index_count = 1;
  texture_buffer_info.p_queue_family_indices = &p_device->get_transfer_family();

  // TODO: make buffer at runtime specifically for transfer commands
  auto buffer = mem::CPUBuffer();
  buffer.init(p_physical_device, p_device, texture_buffer_info);
  buffer.map(image.buffer_size, &image.buffer[0]);

  // map from buffer to image
  texture_images[texture_images.size() - 1][0].transfer(
	  vk::ImageLayout::eTransferDstOptimal, p_device->get_transfer_queue());

  VkOffset3D image_offset = {0, 0, 0};
  texture_images[texture_images.size() - 1][0].copy_from_buffer(
	  buffer.get(), image_offset, std::nullopt, p_device->get_transfer_queue());

  texture_images[texture_images.size() - 1][0].transfer(
	  vk::ImageLayout::eShaderReadOnlyOptimal, p_device->get_graphics_queue());

  buffer.destroy();

  // save to resource set
  mem::Image &vulkan_image = texture_images[texture_images.size() - 1][0];
  ResourceCollection &set = texture_sets[i];
  set.addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulkan_image,
			   texture_sampler);
  set.updateSet(j);
}
*/

void GraphicsImpl::create_texture_image(std::string texturePath, size_t object,
										size_t texture_set)
{
	int imageWidth, imageHeight, imageChannels;
	// this is a char... why is that?
	stbi_uc* pixels = stbi_load(texturePath.c_str(), &imageWidth, &imageHeight,
								&imageChannels, STBI_rgb_alpha);

	VkDeviceSize dataSize = 4 * (imageWidth * imageHeight);
	mem::BufferCreateInfo texture_buffer_info{};
	texture_buffer_info.size = dataSize;
	texture_buffer_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
	texture_buffer_info.memory_properties =
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent;
	texture_buffer_info.queue_family_index_count = 1;
	texture_buffer_info.p_queue_family_indices = &p_device->get_graphics_family();

	// TODO: make buffer at runtime specifically for transfer commands
	auto buffer = mem::CPUBuffer();
	buffer.init(*p_physical_device, *p_device, texture_buffer_info);
	buffer.map(dataSize, 0, pixels);

	// use size of loaded image to create VkImage
	texture_images.resize(texture_images.size() + 1);
	texture_images[texture_images.size() - 1].resize(1);

	br::ImageCreateInfo imageInfo{};
	imageInfo.format = vk::Format::eR8G8B8A8Srgb;
	VkExtent3D extent{};
	extent.width = static_cast<uint32_t>(imageWidth);
	extent.height = static_cast<uint32_t>(imageHeight);
	extent.depth = 1; // this depth might be key to blending the textures...
	imageInfo.extent = extent;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.usage =
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
	imageInfo.queueFamilyIndexCount = 1;
	imageInfo.pQueueFamilyIndices = &p_device->get_transfer_family();
	imageInfo.memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

	br::ImageViewCreateInfo viewInfo{};
	viewInfo.aspect_mask = vk::ImageAspectFlagBits::eColor;

	br::ImageData data{};
	data.name = "texture";
	data.image_info = imageInfo;
	data.image_view_info = viewInfo;

	texture_images[texture_images.size() - 1][0].init(p_physical_device, p_device,
													  data);

	// mem::maAllocateMemory(dataSize, &newTextureImage);
	// transfer the image to appropriate layout for copying

	texture_images[texture_images.size() - 1][0].transfer(
		vk::ImageLayout::eTransferDstOptimal, p_device->get_transfer_queue());

	VkOffset3D image_offset = { 0, 0, 0 };
	texture_images[texture_images.size() - 1][0].copy_from_buffer(
		buffer.get(), image_offset, 1, dataSize, std::nullopt, p_device->get_transfer_queue());

	texture_images[texture_images.size() - 1][0].transfer(
		vk::ImageLayout::eShaderReadOnlyOptimal, p_device->get_graphics_queue());

	// transfer the image again to a more optimal layout for texture sampling?

	// create image view for image
	buffer.destroy();

	// save texture image to mesh
	mem::ImageSize image_dimensions{};
	image_dimensions.width = static_cast<uint32_t>(imageWidth);
	image_dimensions.height = static_cast<uint32_t>(imageHeight);

	// write to texture set
	// write_to_texture_set(texture_sets[object][texture_set],
	// texture_images[texture_images.size() - 1][0].get_api_image_view());
	br::Image& image = texture_images[texture_images.size() - 1][0];
	ResourceCollection& set = texture_sets[object];
	set.addImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image,
				 texture_sampler);
	set.updateSet(texture_set);

	// free image
	stbi_image_free(pixels);
}
