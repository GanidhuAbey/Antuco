#include <bedrock/shader_text.hpp>

#include "logger/interface.hpp"
#include "config.hpp"
#include "shaderc/shaderc.h"

#include <iostream>
#include <string>


using namespace tuco;

ShaderText::ShaderText(std::string shader_code_path, ShaderKind kind) {
    auto code = read_file(shader_code_path);
    std::string string_code(code.begin(), code.end());
    
    shaderc_shader_kind shader_kind;
    switch(kind) {
        case ShaderKind::VERTEX_SHADER:
            shader_kind = shaderc_vertex_shader;
            break;
        case ShaderKind::FRAGMENT_SHADER:
            shader_kind = shaderc_fragment_shader;
            break;
        case ShaderKind::COMPUTE_SHADER:
            shader_kind = shaderc_compute_shader;
            break;

    }

    m_compiled_code = compile_file_to_spirv("shader", shader_kind, string_code);
    create_layouts();
}

ShaderText::~ShaderText() {

}

std::vector<uint32_t>& ShaderText::get_code() {
    return m_compiled_code;
}

std::vector<uint32_t> ShaderText::compile_file_to_spirv(const std::string& source_name,
                                     shaderc_shader_kind kind,
                                     const std::string& source,
                                     bool optimize) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    // Like -DMY_DEFINE=1
    options.AddMacroDefinition("MY_DEFINE", "1");
    if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

    shaderc::SpvCompilationResult module =
        compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << module.GetErrorMessage();
        return std::vector<uint32_t>();
    }

    return {module.cbegin(), module.cend()};
}

// Shamelessly copied from SPIRV-Reflect sample :)
void ShaderText::create_layouts() {
    SpvReflectShaderModule module = {};
    SpvReflectResult result = spvReflectCreateShaderModule(
        m_compiled_code.size() * sizeof(uint32_t), 
        m_compiled_code.data(), 
        &module);

    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    // Demonstrates how to generate all necessary data structures to create a
    // VkDescriptorSetLayout for each descriptor set in this shader.
    m_layout_data.resize(sets.size());
    m_layout_data.insert(
        m_layout_data.begin(), 
        sets.size(), 
        v::DescriptorSetLayoutData{}
    );

    for (size_t i_set = 0; i_set < sets.size(); ++i_set) {
        const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
        v::DescriptorSetLayoutData& layout = m_layout_data[i_set];
        layout.bindings.resize(refl_set.binding_count);
        for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding) {
        const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
        VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
        layout_binding.binding = refl_binding.binding;
        layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
        layout_binding.descriptorCount = 1;

        for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim) {
            layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];
        }
        layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
        }
        layout.set_number = refl_set.set;
        layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout.create_info.bindingCount = refl_set.binding_count;
        layout.create_info.pBindings = layout.bindings.data();
    }

    spvReflectDestroyShaderModule(&module);
}
