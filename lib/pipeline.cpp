#include "pipeline.hpp"
#include "shader_text.hpp"

#include "logger/interface.hpp"

#include <stdexcept>


using namespace tuco;

//PURPOSE: create a vulkan pipeline with desired configuration
//PARAMETERS:
//<PipelineConfig config> - configuration settings for pipeline
void TucoPipeline::init(VkDevice device, PipelineConfig config) {
    api_device = device;

    //distinguish render/compute pipeline
    if (config.compute_shader_path.has_value()) {
        //build compute shader
        create_compute_pipeline(config);
    } else {
        create_render_pipeline(config);
    }
}

TucoPipeline::TucoPipeline() {}

TucoPipeline::~TucoPipeline() {
    //if (api_device != VK_NULL_HANDLE) destroy();
}

void TucoPipeline::destroy() {
    vkDestroyPipeline(api_device, pipeline, nullptr);
    vkDestroyPipelineLayout(api_device, layout, nullptr);
}

VkPipeline TucoPipeline::get_api_pipeline() {
    return pipeline;
}

VkPipelineLayout TucoPipeline::get_api_layout() {
    return layout;
}


VkShaderModule TucoPipeline::create_shader_module(std::vector<uint32_t> shaderCode) {
    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);

    createInfo.pCode = shaderCode.data();

    if (vkCreateShaderModule(api_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("could not create shader module");
    }

    return shaderModule;
}

VkPipelineShaderStageCreateInfo TucoPipeline::fill_shader_stage_struct(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
    VkPipelineShaderStageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = stage;
    createInfo.module = shaderModule;
    createInfo.pName = "main";

    return createInfo;
}

void TucoPipeline::create_compute_pipeline(PipelineConfig config) {
    VkShaderModule compute_shader;
    VkPipelineShaderStageCreateInfo shader_info;

    ShaderText compute_code(config.compute_shader_path.value(), ShaderKind::COMPUTE_SHADER); 
    compute_shader = create_shader_module(compute_code.get_code());
    shader_info = fill_shader_stage_struct(VK_SHADER_STAGE_COMPUTE_BIT, compute_shader);

    create_pipeline_layout(config.descriptor_layouts, config.push_ranges, &layout);

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = nullptr;
    pipeline_info.flags = 0;
    pipeline_info.stage = shader_info;
    pipeline_info.layout = layout;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;


    if (vkCreateComputePipelines(api_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
        LOG("could not create compute shader");
    }

}

VkPipelineColorBlendAttachmentState TucoPipeline::enable_alpha_blending() {
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    return color_blend_attachment;
}

void TucoPipeline::create_render_pipeline(PipelineConfig config) {
    //might have two might have one
    VkShaderModule vert_shader;
    VkShaderModule frag_shader;
    VkPipelineShaderStageCreateInfo vert_shader_info;
    VkPipelineShaderStageCreateInfo frag_shader_info;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

    if (config.vert_shader_path.has_value()) {
        ShaderText vert_code(config.vert_shader_path.value(), ShaderKind::VERTEX_SHADER);
        vert_shader = create_shader_module(vert_code.get_code());
        vert_shader_info = fill_shader_stage_struct(VK_SHADER_STAGE_VERTEX_BIT, vert_shader);
        shader_stages.push_back(vert_shader_info);
    }
    if (config.frag_shader_path.has_value()) {
        ShaderText frag_code(config.frag_shader_path.value(), ShaderKind::FRAGMENT_SHADER);
        frag_shader = create_shader_module(frag_code.get_code());
        frag_shader_info = fill_shader_stage_struct(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader);
        shader_stages.push_back(frag_shader_info);
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = config.binding_descriptions.size(); 
    vertex_input_info.pVertexBindingDescriptions = config.binding_descriptions.data();

    vertex_input_info.vertexAttributeDescriptionCount = config.attribute_descriptions.size();
    vertex_input_info.pVertexAttributeDescriptions = config.attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)config.screen_extent.width;
    viewport.height = (float)config.screen_extent.height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    VkRect2D scissor{   };
    scissor.offset = { 0, 0 };
    scissor.extent = config.screen_extent;
 
    VkPipelineViewportStateCreateInfo viewport_info{};

    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports = &viewport;
    viewport_info.scissorCount = 1;
    viewport_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_info{};
    rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_info.depthClampEnable = VK_FALSE;
    rasterization_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_info.lineWidth = 1.0f;
    rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_info.depthBiasEnable = config.depth_bias_enable;
    rasterization_info.depthBiasConstantFactor = 0.0f; // Optional
    rasterization_info.depthBiasClamp = 0.0f; // Optional
    rasterization_info.depthBiasSlopeFactor = 0.0f; // Optional
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional
    
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | 
        VK_COLOR_COMPONENT_G_BIT | 
        VK_COLOR_COMPONENT_B_BIT | 
        VK_COLOR_COMPONENT_A_BIT;

    color_blend_attachment.blendEnable = VK_FALSE;

    if (config.blend_colours) {
        color_blend_attachment = enable_alpha_blending();
    }

    VkPipelineColorBlendStateCreateInfo color_blend_info{};
    color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
    depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_info.pNext = nullptr;
    depth_stencil_info.flags = 0;
    depth_stencil_info.depthTestEnable = VK_TRUE;
    depth_stencil_info.depthWriteEnable = VK_TRUE;
    depth_stencil_info.depthCompareOp = config.depth_compare_op;
    depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_info.stencilTestEnable = VK_FALSE;

    float blendValues[4] = { 0.0, 0.0, 0.0, 0.5 };
    color_blend_info.blendConstants[0] = blendValues[0];
    color_blend_info.blendConstants[1] = blendValues[1];
    color_blend_info.blendConstants[2] = blendValues[2];
    color_blend_info.blendConstants[3] = blendValues[3];

    VkPipelineDynamicStateCreateInfo dynamic_info{};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount = static_cast<uint32_t>(config.dynamic_states.size());
    dynamic_info.pDynamicStates = config.dynamic_states.data();
    
    create_pipeline_layout(config.descriptor_layouts, config.push_ranges, &layout);

    VkGraphicsPipelineCreateInfo create_pipeline_info{};
    create_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
    create_pipeline_info.pStages = shader_stages.data();
    create_pipeline_info.pVertexInputState = &vertex_input_info;
    create_pipeline_info.pInputAssemblyState = &input_assembly_info;
    create_pipeline_info.pViewportState = &viewport_info;
    create_pipeline_info.pRasterizationState = &rasterization_info;
    create_pipeline_info.pMultisampleState = &multisampling;
    create_pipeline_info.pColorBlendState = &color_blend_info;
    create_pipeline_info.pDepthStencilState = &depth_stencil_info;
    create_pipeline_info.pDynamicState = &dynamic_info;
    create_pipeline_info.layout = layout;
    create_pipeline_info.renderPass = config.pass;
    create_pipeline_info.subpass = config.subpass_index;
    
    VkResult result = vkCreateGraphicsPipelines(api_device, VK_NULL_HANDLE, 1, &create_pipeline_info, nullptr, &pipeline);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("could not create graphics pipeline");
    }

    //destroy the used shader object
    if (config.vert_shader_path.has_value()) vkDestroyShaderModule(api_device, vert_shader, nullptr);
    if (config.frag_shader_path.has_value()) vkDestroyShaderModule(api_device, frag_shader, nullptr);

}


void TucoPipeline::create_pipeline_layout(std::vector<VkDescriptorSetLayout> descriptor_layouts, std::vector<VkPushConstantRange> push_ranges, VkPipelineLayout* layout) {
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_layouts.size());
    pipeline_layout_info.pSetLayouts = descriptor_layouts.data();

    //as long as this is zero the pointer data sent in will be ignored, so the user can send an empty vector and it wont matter
    pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_ranges.size());
    pipeline_layout_info.pPushConstantRanges = push_ranges.data();

    if (vkCreatePipelineLayout(api_device, &pipeline_layout_info, nullptr, layout) != VK_SUCCESS) {
        throw std::runtime_error("could not create pipeline layout");
    }
}
