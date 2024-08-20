//compiles shader code to spir-v.
#pragma once

#include <unordered_map>

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include <vulkan_wrapper/descriptor_layout.hpp>

// This file defines some functions for printing the reflect data.
// useful as reference, please refer to the SPIRV-Reflect github.
// https://github.com/KhronosGroup/SPIRV-Reflect
//#include <common.h>

namespace br {

enum class ShaderKind {
    VertexShader = shaderc_vertex_shader,
    FragmentShader = shaderc_fragment_shader,
    ComputeShader = shaderc_compute_shader,
};

class ShaderText {
private:
    std::unordered_map<ShaderKind, std::vector<uint32_t>> compiled_code;

    // from shader we extract layouts for all descriptors
    // since we have multiple sets per shader (e.g draw, material, scene, etc) we need multiple layouts.
    std::vector<v::DescriptorLayout> layouts;


public:
    //requires: shader_code_path must refer to valid file within project directory
    // % deprecated %
    ShaderText(std::string shader_code_path, ShaderKind kind);

    ShaderText() = default;
    ~ShaderText();

    //returns compiled code as string
    std::vector<uint32_t>& get_code(ShaderKind kind);

    v::DescriptorLayout& get_layout(uint32_t i) { return layouts[i]; }

    // ShaderText supports the ability to compile multiple shaders, but the expectation is that the given shaders
    // are part of a single PSO (and hence would share descriptor layouts)
    void compile(std::string shader_code_path, ShaderKind kind);
    void create_layouts(std::shared_ptr<v::Device> device);

private:
    // Compiles a shader to SPIR-V assembly. Returns the assembly text
    // as a string.
    std::vector<uint32_t> compile_file_to_spirv(const std::string& source_name,
                                     shaderc_shader_kind kind,
                                     const std::string& source,
                                     bool optimize = false);

    shaderc_shader_kind get_shaderc_kind(ShaderKind kind);

    std::vector<v::DescriptorLayout> collect_layouts(std::vector<uint32_t> &code);

};
};
