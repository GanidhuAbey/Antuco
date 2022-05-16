//an api wrapper for vulkan pipeline

#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>
#include <optional>
#include <string>

namespace tuco {

struct PipelineConfig {
    VkExtent2D screen_extent;
    std::optional<std::string> vert_shader_path;
    std::optional<std::string> frag_shader_path;
    std::vector<VkDynamicState> dynamic_states;
    std::vector<VkDescriptorSetLayout> descriptor_layouts;
    std::vector<VkPushConstantRange> push_ranges;
    VkRenderPass pass;
    uint32_t subpass_index;
};

class TucoPipeline {
    private:
        VkPipeline pipeline;
        VkPipelineLayout layout;

        std::shared_ptr<VkDevice> p_device;
    public:
        TucoPipeline(VkDevice device, PipelineConfig config);
        ~TucoPipeline();

        VkPipeline get_api_pipeline();

    private:
        void create_render_pipeline(PipelineConfig config);
        void create_compute_pipeline();


        VkShaderModule create_shader_module(std::vector<uint32_t> shaderCode);
        VkPipelineShaderStageCreateInfo fill_shader_stage_struct(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
        void create_pipeline_layout(std::vector<VkDescriptorSetLayout> descriptor_layouts, std::vector<VkPushConstantRange> push_ranges, VkPipelineLayout* layout);
};

}
