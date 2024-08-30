#include "material_type.hpp"
#include "vulkan_wrapper/device.hpp"

using namespace tuco;

void MaterialType::buildPipeline(v::Device& device) {
    PipelineConfig config{};
    config.vert_shader_path = vertexShaderPath;
    config.frag_shader_path = fragmentShaderPath;
    config.descriptor_layouts = setLayouts;
    config.push_ranges = pushRanges;
    config.pass = renderPass.get_api_pass();
    config.subpass_index = 0; // TODO: wtf does this do again lmao
    config.screen_extent = screenExtent;
    config.blend_colours = blend;

    //pso.init(device, config);

    psoInitialized = true;
}

void MaterialType::setVertexShader(std::string shaderPath) {
    vertexShaderPath = shaderPath;
}
void MaterialType::setFragmentShader(std::string shaderPath) {
    fragmentShaderPath = shaderPath;
}

void MaterialType::setDescriptorLayouts(std::vector<VkDescriptorSetLayout> &&layouts) {
    setLayouts = layouts;
}

void MaterialType::addPushRange(VkPushConstantRange range) {
    pushRanges.push_back(range);
}

void MaterialType::setRenderPass(TucoPass &pass) {
    renderPass = pass;
}

void MaterialType::setScreenExtent(VkExtent2D extent) {
    screenExtent = extent;
}

void MaterialType::setBlend(bool blend) {
    MaterialType::blend = blend;
}
