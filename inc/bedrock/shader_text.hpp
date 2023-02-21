//compiles shader code to spir-v.
#pragma once

#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>
#include <common.h>
#include <vulkan/vulkan.hpp>

#include <vulkan_wrapper/binding_layout.hpp>

// TODO: change to br
namespace br {

enum class ShaderKind {
    VERTEX_SHADER = shaderc_vertex_shader,
    FRAGMENT_SHADER = shaderc_fragment_shader,
    COMPUTE_SHADER = shaderc_compute_shader,

    VERTEX_FRAGMENT_COMBINED
};

class ShaderText {
private:
    std::vector<uint32_t> m_compiled_code;
    std::vector<v::DescriptorSetLayoutData> m_layout_data;

    std::unique_ptr<v::BindingLayouts> mp_layouts;

    ShaderKind m_kind;

    std::string m_define;

    // initializers
    bool m_layouts_built;

public:
    //requires: shader_code_path must refer to valid file within project directory
    ShaderText(std::string shader_code_path, ShaderKind kind);
    ~ShaderText();

    //returns compiled code as string
    std::vector<uint32_t>& get_code();
    std::vector<v::DescriptorSetLayoutData>& get_layout_data() {
        return m_layout_data;
    }

    void build_layouts(v::Device& device);
    VkDescriptorSetLayout& get_layout(uint32_t index);

    std::vector<VkDescriptorSetLayout>& get_layouts() {
        assert(mp_layouts);

        return mp_layouts->get_layouts();
    }

    uint32_t get_layout_count() const {
        return static_cast<uint32_t>(m_layout_data.size());
    }
    
private:
    // Compiles a shader to SPIR-V assembly. Returns the assembly text
    // as a string.
    std::vector<uint32_t> compile_file_to_spirv(const std::string& source_name,
                                     shaderc_shader_kind kind,
                                     const std::string& source,
                                     bool optimize = false);

    // any shaders compiled through this file is expected to be a 
    // vertex & fragment shader in a single file, seperated via the 
    // defines "VERTEX" and "FRAGMENT"

    /*
    // with the introduction of this method, a ShaderText object may contain
    // multiple shaders.
    std::vector<uint32_t> compile_render_shader(const std::string& file_name,
        const std::string& source,
        ShaderKind kind);
    */

    void create_layout_data();
};
};
