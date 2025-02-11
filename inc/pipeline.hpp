#pragma once

//an api wrapper for vulkan pipeline
//
#include "data_structures.hpp"
#include <bedrock/shader_text.hpp>
#include <descriptor_set.hpp>

#include <memory>
#include <vulkan/vulkan.hpp>

#include "vulkan_wrapper/device.hpp"

#include <vector>
#include <optional>
#include <string>

namespace tuco
{


/* ---- parameters which control pipeline creation --------
 *  screen_extent : dimensions of the screen being rendered to
 *  vert_shader_path : optional path to the vertex shader
 *  frag_shader_path : optional path to the fragment shader
 *  compute_shader_path : optional path to the compute shader, if this value is filled in,
 *                        then pipeline will be assumed to be a compute pipeline.
 *  dynamic_states : list of pipeline states that the user would like to control
 *      from the command buffers (read doc on VkDynamicState for more info)
 *  descriptor_layouts : descriptor sets specify how data is read into the shader, the layouts
 *      the layouts will control what descriptor sets the pipeline is able to use.
 *  push_ranges : a list which contains the relavent data for all push constants being
 *      used in the pipeline
 *  pass : render passes can use multiple pipelines, but pipelines must state which renderpass
 *      it can be used be, this parameter will set that.
 *  subpass_index : similiar to the above this will control which subpass within the renderpass
 *      the pipeline will be used by.
 *  blend_colours : when set to true, :the alpha value of a fragment will be accounted for when computing
 *      the final colour of a pixel.
 *
 * --------------------------------------------------------------------------------
*/

struct PipelineConfig
{
	vk::Extent2D screen_extent;
	std::optional<std::string> vert_shader_path = std::nullopt;
	std::optional<std::string> frag_shader_path = std::nullopt;
	std::optional<std::string> compute_shader_path = std::nullopt;
	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS,
	};
	VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS;
	VkBool32 depth_bias_enable = VK_FALSE;
	VkBool32 depth_test_enable = VK_TRUE;
	VkRenderPass pass;
	uint32_t subpass_index;
	bool blend_colours = VK_FALSE;

	std::vector<VkPushConstantRange> push_ranges = std::vector<VkPushConstantRange>(0);

	std::vector<vk::VertexInputBindingDescription>
		binding_descriptions = {
			vk::VertexInputBindingDescription({
				0,
				sizeof(Vertex),
				VK_VERTEX_INPUT_RATE_VERTEX})
	};

	//attribute description
	std::vector<vk::VertexInputAttributeDescription>
		attribute_descriptions = {
			vk::VertexInputAttributeDescription({
					0,
					0,
					VK_FORMAT_R32G32B32_SFLOAT,
					offsetof(Vertex, position)}), //position
			vk::VertexInputAttributeDescription({
					1,
					0,
					VK_FORMAT_R32G32B32_SFLOAT,
					offsetof(Vertex, normal)}), //normal
			vk::VertexInputAttributeDescription({
					2,
					0,
					VK_FORMAT_R32G32_SFLOAT,
					offsetof(Vertex, tex_coord)}) //texture coord
	};

	VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
	VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
};

class TucoPipeline
{
private:
	vk::Pipeline pipeline_;
	vk::PipelineLayout layout_;

	br::ShaderText shader_compiler;

	std::shared_ptr<v::Device> api_device;

	std::unordered_map<uint32_t, ResourceCollection> resource_collections;

	std::shared_ptr<mem::Pool> set_pool;

public:
	void init(std::shared_ptr<v::Device> device, std::shared_ptr<mem::Pool> pool, const PipelineConfig& config);

	VkPipeline get_api_pipeline();
	VkPipelineLayout get_api_layout();

	void destroy();

	// Get specific resource type (eventually once shaders restructured, 0 = draw, 1 = material, 2 = pass, 3 = scene)
	ResourceCollection* get_resource_collection(uint32_t type);

private:
	void create_render_pipeline(const PipelineConfig& config);
	void create_compute_pipeline(const PipelineConfig& config);

	void create_resource_collections();

//helper functions
private:

	VkPipelineColorBlendAttachmentState enable_alpha_blending();
	vk::ShaderModule create_shader_module(std::vector<uint32_t> shaderCode);
	vk::PipelineShaderStageCreateInfo fill_shader_stage_struct(
		vk::ShaderStageFlagBits stage,
		vk::ShaderModule shaderModule
	);
	void create_pipeline_layout(
		const std::vector<VkPushConstantRange>& push_ranges);
};

}
