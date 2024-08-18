//compiles shader code to spir-v.
#pragma once

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

// This file defines some functions for printing the reflect data.
// useful as reference, please refer to the SPIRV-Reflect github.
// https://github.com/KhronosGroup/SPIRV-Reflect
//#include <common.h>

namespace tuco {

enum class ShaderKind {
    VERTEX_SHADER = shaderc_vertex_shader,
    FRAGMENT_SHADER = shaderc_fragment_shader,
    COMPUTE_SHADER = shaderc_compute_shader,
};

class ShaderText {
private:
    std::vector<uint32_t> compiled_code;

    // from shader we extract layouts for all descriptors
    // since we have multiple sets per shader (e.g draw, material, scene, etc) we need multiple layouts.
    std::vector<VkDescriptorSetLayout> m_layouts;


public:
    //requires: shader_code_path must refer to valid file within project directory
    ShaderText(std::string shader_code_path, ShaderKind kind);
    ~ShaderText();

    //returns compiled code as string
    std::vector<uint32_t> get_code();

    VkDescriptorSetLayout get_layout(uint32_t i) { return m_layouts[i]; }

private:
    // Compiles a shader to SPIR-V assembly. Returns the assembly text
    // as a string.
    std::vector<uint32_t> compile_file_to_spirv(const std::string& source_name,
                                     shaderc_shader_kind kind,
                                     const std::string& source,
                                     bool optimize = false);

    void create_layouts();

};
};
