#pragma once

#include "vulkan/vulkan_core.h"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "api_config.hpp"

namespace tuco {
class MaterialType {
private:
    TucoPipeline pso;
    bool psoInitialized = false;

    std::string vertexShaderPath;
    std::string fragmentShaderPath;

    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange> pushRanges;

    VkExtent2D screenExtent;
    bool blend = true;

    TucoPass renderPass;

public:
    MaterialType() = default;
    ~MaterialType();

    void buildPipeline(v::Device &device);
    void setVertexShader(std::string shaderPath);
    void setFragmentShader(std::string shaderPath);

    // TODO: automate with shader reflection
    void setDescriptorLayouts(std::vector<VkDescriptorSetLayout>&& layouts);
    void addPushRange(VkPushConstantRange range);
    void setRenderPass(TucoPass& pass);
    void setScreenExtent(VkExtent2D extent);
    void setBlend(bool blend);

};
}
