#include "pipeline.hpp"

#include "logger/interface.hpp"

#include <stdexcept>


using namespace tuco;

ResourceCollection* TucoPipeline::get_resource_collection(uint32_t type)
{
	if (auto search = resource_collections.find(type); search != resource_collections.end())
	{
		return &search->second;
	}

	ASSERT(false, "could not find resource group of type {}", type);
}

void TucoPipeline::create_resource_collections()
{
	std::vector<v::DescriptorLayout>& layouts = shader_compiler.get_layouts();

	for (auto& layout : layouts)
	{
		ResourceCollection collection(api_device.get(), layout.get_api_layout());

		// TODO : in some cases we may want to have multiple sets per layout, need to consider whether we'll need to optionally pass set number.
		//collection.addSets(1, *set_pool);
		resource_collections[layout.get_index()] = std::move(collection);
	}
}

//PURPOSE: create a vulkan pipeline with desired configuration
//PARAMETERS:
//<PipelineConfig config> - configuration settings for pipeline
void TucoPipeline::init(std::shared_ptr<v::Device> device, std::shared_ptr<mem::Pool> pool, const PipelineConfig& config)
{
	api_device = device;
	set_pool = pool;

	//distinguish render/compute pipeline
	if (config.compute_shader_path.has_value())
	{
//build compute shader
		create_compute_pipeline(config);
	}
	else
	{
		create_render_pipeline(config);
	}

	create_resource_collections();
}

void TucoPipeline::destroy()
{
	api_device->get().destroyPipeline(pipeline_);
	api_device->get().destroyPipelineLayout(layout_);
}

VkPipeline TucoPipeline::get_api_pipeline()
{
	return pipeline_;
}

VkPipelineLayout TucoPipeline::get_api_layout()
{
	return layout_;
}


vk::ShaderModule TucoPipeline::create_shader_module(std::vector<uint32_t> shaderCode)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);

	createInfo.pCode = shaderCode.data();

	auto create = vk::ShaderModuleCreateInfo(
		{},
		static_cast<size_t>(shaderCode.size() * sizeof(uint32_t)),
		shaderCode.data()
	);

	auto shader_module = api_device->get().createShaderModule(create);

	return shader_module;
}

vk::PipelineShaderStageCreateInfo TucoPipeline::fill_shader_stage_struct(vk::ShaderStageFlagBits stage, vk::ShaderModule shaderModule)
{
	auto create = vk::PipelineShaderStageCreateInfo(
		{},
		stage,
		shaderModule,
		"main"
	);

	return create;
}

void TucoPipeline::create_compute_pipeline(const PipelineConfig& config)
{
	vk::ShaderModule compute_shader;
	vk::PipelineShaderStageCreateInfo shader_info;

	br::ShaderText compute_code(config.compute_shader_path.value(), br::ShaderKind::ComputeShader);
	compute_shader = create_shader_module(compute_code.get_code(br::ShaderKind::ComputeShader));
	shader_info = fill_shader_stage_struct(vk::ShaderStageFlagBits::eCompute, compute_shader);

	//create_pipeline_layout(config.descriptor_layouts, config.push_ranges);

	auto pipeline_info = vk::ComputePipelineCreateInfo(
		{},
		shader_info,
		layout_
	);


	pipeline_ = api_device->get().createComputePipeline(VK_NULL_HANDLE, pipeline_info).value;
}

VkPipelineColorBlendAttachmentState TucoPipeline::enable_alpha_blending()
{
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	return color_blend_attachment;
}

void TucoPipeline::create_render_pipeline(const PipelineConfig& config)
{
//might have two might have one
	vk::ShaderModule vert_shader;
	vk::ShaderModule frag_shader;
	vk::PipelineShaderStageCreateInfo vert_shader_info;
	vk::PipelineShaderStageCreateInfo frag_shader_info;

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

	if (config.vert_shader_path.has_value())
	{
		shader_compiler.compile(config.vert_shader_path.value(), br::ShaderKind::VertexShader);
		//br::ShaderText vert_code(config.vert_shader_path.value(), br::ShaderKind::VertexShader);
		vert_shader = create_shader_module(shader_compiler.get_code(br::ShaderKind::VertexShader));
		vert_shader_info = fill_shader_stage_struct(vk::ShaderStageFlagBits::eVertex, vert_shader);
		shader_stages.push_back(vert_shader_info);
	}
	if (config.frag_shader_path.has_value())
	{
		shader_compiler.compile(config.frag_shader_path.value(), br::ShaderKind::FragmentShader);
		//br::ShaderText frag_code(config.frag_shader_path.value(), br::ShaderKind::FragmentShader);
		frag_shader = create_shader_module(shader_compiler.get_code(br::ShaderKind::FragmentShader));
		frag_shader_info = fill_shader_stage_struct(vk::ShaderStageFlagBits::eFragment, frag_shader);
		shader_stages.push_back(frag_shader_info);
	}

	shader_compiler.create_layouts(api_device);

	auto viewport = vk::Viewport(
		0,
		0,
		static_cast<float>(config.screen_extent.width),
		static_cast<float>(config.screen_extent.height),
		0.f,
		1.f
	);


	auto scissor = vk::Rect2D(
		vk::Offset2D(0, 0),
		config.screen_extent
	);

	VkPipelineMultisampleStateCreateInfo multisampling_old{};
	multisampling_old.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_old.sampleShadingEnable = VK_FALSE;
	multisampling_old.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling_old.minSampleShading = 1.0f; // Optional
	multisampling_old.pSampleMask = nullptr; // Optional
	multisampling_old.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling_old.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	color_blend_attachment.blendEnable = VK_FALSE;

	if (config.blend_colours)
	{
		color_blend_attachment = enable_alpha_blending();
	}

	VkPipelineColorBlendStateCreateInfo color_blend_info_old{};
	color_blend_info_old.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_info_old.logicOpEnable = VK_FALSE;
	color_blend_info_old.attachmentCount = 1;
	color_blend_info_old.pAttachments = &color_blend_attachment;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_info_old{};
	depth_stencil_info_old.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_info_old.pNext = nullptr;
	depth_stencil_info_old.flags = 0;
	depth_stencil_info_old.depthTestEnable = config.depth_test_enable;
	depth_stencil_info_old.depthWriteEnable = config.depth_test_enable;
	depth_stencil_info_old.depthCompareOp = config.depth_compare_op;
	depth_stencil_info_old.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_info_old.stencilTestEnable = VK_FALSE;

	float blendValues[4] = { 0.0, 0.0, 0.0, 0.5 };
	color_blend_info_old.blendConstants[0] = blendValues[0];
	color_blend_info_old.blendConstants[1] = blendValues[1];
	color_blend_info_old.blendConstants[2] = blendValues[2];
	color_blend_info_old.blendConstants[3] = blendValues[3];

	VkPipelineDynamicStateCreateInfo dynamic_info_old{};
	dynamic_info_old.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_info_old.dynamicStateCount = static_cast<uint32_t>(config.dynamic_states.size());
	dynamic_info_old.pDynamicStates = config.dynamic_states.data();

	auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo(
		{},
		static_cast<uint32_t>(config.binding_descriptions.size()),
		config.binding_descriptions.data(),
		static_cast<uint32_t>(config.attribute_descriptions.size()),
		config.attribute_descriptions.data()
	);

	auto input_assembly_info = vk::PipelineInputAssemblyStateCreateInfo(
		{},
		vk::PrimitiveTopology::eTriangleList,
		false
	);

	auto viewport_info = vk::PipelineViewportStateCreateInfo({},
															 1,
															 &viewport,
															 1,
															 &scissor
	);

	auto rasterization_info = vk::PipelineRasterizationStateCreateInfo(
		{},
		false,
		false,
		vk::PolygonMode::eFill,
		vk::CullModeFlags(config.cull_mode),
		vk::FrontFace(config.front_face),
		config.depth_bias_enable,
		0.f,
		0.f,
		0.f,
		1.f
	);

	auto multisampling = vk::PipelineMultisampleStateCreateInfo(multisampling_old);
	auto color_blend_info = vk::PipelineColorBlendStateCreateInfo(color_blend_info_old);
	auto depth_stencil_info = vk::PipelineDepthStencilStateCreateInfo(depth_stencil_info_old);
	auto dynamic_info = vk::PipelineDynamicStateCreateInfo(dynamic_info_old);

	create_pipeline_layout(config.push_ranges);

	auto create_info = vk::GraphicsPipelineCreateInfo(
		{},
		static_cast<uint32_t>(shader_stages.size()),
		shader_stages.data(),
		&vertex_input_info,
		&input_assembly_info,
		{},
		&viewport_info,
		&rasterization_info,
		&multisampling,
		&depth_stencil_info,
		&color_blend_info,
		&dynamic_info,
		layout_,
		config.pass,
		config.subpass_index
	);

	pipeline_ = api_device->get().createGraphicsPipeline(VK_NULL_HANDLE, create_info).value;

	//destroy the used shader object
	if (config.vert_shader_path.has_value()) api_device->get().destroyShaderModule(vert_shader);
	if (config.frag_shader_path.has_value()) api_device->get().destroyShaderModule(frag_shader);
}

void TucoPipeline::create_pipeline_layout(
	const std::vector<VkPushConstantRange>& push_ranges)
{
	auto info = VkPipelineLayoutCreateInfo{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.flags = 0;
	info.pNext = nullptr;
	info.setLayoutCount = shader_compiler.get_layout_count();//static_cast<uint32_t>(set_layouts.size());

	std::vector<VkDescriptorSetLayout> descriptor_layouts;

	for (auto& layout : shader_compiler.get_layouts())
	{
		descriptor_layouts.push_back(layout.get_api_layout());
	}

	info.pSetLayouts = descriptor_layouts.data();//set_layouts.data();
	info.pushConstantRangeCount = static_cast<uint32_t>(push_ranges.size());
	info.pPushConstantRanges = push_ranges.data();

	VkPipelineLayout old_layout;
	auto result = vkCreatePipelineLayout(api_device->get(), &info, nullptr, &old_layout);

	layout_ = vk::PipelineLayout(old_layout);

	ASSERT(result == VK_SUCCESS, "could not ceate pipeline layout, error code: {}", static_cast<uint32_t>(result))
}
