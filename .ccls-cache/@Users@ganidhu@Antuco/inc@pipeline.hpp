//an api wrapper for vulkan pipeline
//
#include "data_structures.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

#include <vector>
#include <optional>
#include <string>

namespace tuco {


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
 *  blend_colours : when set to true, the alpha value of a fragment will be accounted for when computing
 *      the final colour of a pixel.
 *  
 * --------------------------------------------------------------------------------
*/


struct PipelineConfig {
    VkExtent2D screen_extent;
    std::optional<std::string> vert_shader_path = std::nullopt;
    std::optional<std::string> frag_shader_path = std::nullopt;
    std::optional<std::string> compute_shader_path = std::nullopt;
    std::vector<VkDynamicState> dynamic_states;
    VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS;
    VkBool32 depth_bias_enable = VK_FALSE;
    VkRenderPass pass;
    uint32_t subpass_index;
    bool blend_colours = VK_FALSE;

    std::vector<VkDescriptorSetLayout> descriptor_layouts;
    std::vector<VkPushConstantRange> push_ranges = std::vector<VkPushConstantRange>(0);

    std::vector<VkVertexInputBindingDescription> binding_descriptions = {
        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
    };

    //attribute description
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)}, //position
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)}, //normal
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex_coord)}, //texture coord
    };

    VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
};

class TucoPipeline {
    private:
        VkPipeline pipeline_;
        VkPipelineLayout layout_;

        VkDevice api_device;
    public:
        void init(VkDevice& device, const PipelineConfig& config);
        TucoPipeline();
        ~TucoPipeline();

        VkPipeline get_api_pipeline();
        VkPipelineLayout get_api_layout();

        void destroy();

    private:
        void create_render_pipeline(const PipelineConfig& config);
        void create_compute_pipeline(const PipelineConfig& config);


    //helper functions
    private:
        
        VkPipelineColorBlendAttachmentState enable_alpha_blending();
        VkShaderModule create_shader_module(std::vector<uint32_t> shaderCode);
        VkPipelineShaderStageCreateInfo fill_shader_stage_struct(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
        void create_pipeline_layout(const std::vector<VkDescriptorSetLayout>& set_layouts,
                                    const std::vector<VkPushConstantRange>& push_ranges);
};

}
