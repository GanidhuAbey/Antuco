//compiles shader code to spir-v.
#pragma once

#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>
#include <vulkan/vulkan.hpp>

namespace tuco {

enum class ShaderKind {
    VERTEX_SHADER = shaderc_vertex_shader,
    FRAGMENT_SHADER = shaderc_fragment_shader,
    COMPUTE_SHADER = shaderc_compute_shader,
};

// TODO : need to move this to vulkan wrapper
struct DescriptorSetLayoutData {
  uint32_t set_number;
  VkDescriptorSetLayoutCreateInfo create_info;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class ShaderText {
private:
    std::vector<uint32_t> m_compiled_code;

public:
    //requires: shader_code_path must refer to valid file within project directory
    ShaderText(std::string shader_code_path, ShaderKind kind);
    ~ShaderText();

    //returns compiled code as string
    std::vector<uint32_t> get_code();

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
