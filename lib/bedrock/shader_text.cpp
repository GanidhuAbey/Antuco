#include <bedrock/shader_text.hpp>

#include <spirv_reflect.h>

#include "logger/interface.hpp"
#include "config.hpp"
#include "shaderc/shaderc.h"

#include <iostream>
#include <string>


using namespace br;

shaderc_shader_kind ShaderText::get_shaderc_kind(ShaderKind kind)
{
    shaderc_shader_kind shader_kind;
    switch (kind)
    {
    case ShaderKind::VertexShader:
        shader_kind = shaderc_vertex_shader;
        break;
    case ShaderKind::FragmentShader:
        shader_kind = shaderc_fragment_shader;
        break;
    case ShaderKind::ComputeShader:
        shader_kind = shaderc_compute_shader;
        break;

    }

    return shader_kind;
}

ShaderText::ShaderText(std::string shader_code_path, ShaderKind kind) {
    auto code = read_file(shader_code_path);
    std::string string_code(code.begin(), code.end());
    
    shaderc_shader_kind shader_kind = get_shaderc_kind(kind);

    compiled_code[kind] = compile_file_to_spirv(get_file_name(shader_code_path), shader_kind, string_code);
}

void ShaderText::compile(std::string shader_path, ShaderKind kind)
{
    auto code = read_file(shader_path);
    std::string string_code(code.begin(), code.end());

    shaderc_shader_kind shader_kind = get_shaderc_kind(kind);

    compiled_code[kind] = compile_file_to_spirv(get_file_name(shader_path), shader_kind, string_code);
}

ShaderText::~ShaderText() {

}

std::vector<uint32_t>& ShaderText::get_code(ShaderKind kind) {
    if (auto search = compiled_code.find(kind); search != compiled_code.end())
    {
        return search->second;
    }
    ASSERT(false, "No compiled shader available of kind {}", static_cast<uint32_t>(kind));
}

std::vector<uint32_t> ShaderText::compile_file_to_spirv(const std::string& source_name,
                                     shaderc_shader_kind kind,
                                     const std::string& source,
                                     bool optimize) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  // Like -DMY_DEFINE=1
  // Can optionally use this to define macros which contain vertex and pixel shader in same file (but separated)
  //options.AddMacroDefinition("MY_DEFINE", "1");
  options.AddMacroDefinition("DRAW_SET", "0");
  options.AddMacroDefinition("MATERIAL_SET", "1");
  options.AddMacroDefinition("PASS_SET", "2");
  options.AddMacroDefinition("SCENE_SET", "3");
  
  if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

  shaderc::SpvCompilationResult module =
      compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

  if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
    std::cerr << module.GetErrorMessage();
    return std::vector<uint32_t>();
  }

  return {module.cbegin(), module.cend()};
}

// [TODO 8/2024] - Instead of using 0,1,2,3 for set numbers, we should use terms like draw,material,pass,scene etc and then use a define to convert these to set values
//                 This could eliminate accidental errors and create stronger correllation between set number and type of data.

void ShaderText::create_layouts(std::shared_ptr<v::Device> device) {
    // vertex shader
    std::vector<v::Binding> shader_bindings;
    if (auto search = compiled_code.find(ShaderKind::VertexShader); search != compiled_code.end())
    {
        std::vector<v::Binding> vertex_bindings = collect_layouts(search->second);
        shader_bindings.insert(shader_bindings.end(), vertex_bindings.begin(), vertex_bindings.end());
    }

    // fragment shader
    if (auto search = compiled_code.find(ShaderKind::FragmentShader); search != compiled_code.end())
    {
        std::vector<v::Binding> fragment_bindings = collect_layouts(search->second);
        shader_bindings.insert(shader_bindings.end(), fragment_bindings.begin(), fragment_bindings.end());
    }

    // compute shader (should only have it set if vertex & fragment not set)
    if (auto search = compiled_code.find(ShaderKind::ComputeShader); search != compiled_code.end())
    {
        std::vector<v::Binding> compute_bindings = collect_layouts(search->second);
        shader_bindings.insert(shader_bindings.end(), compute_bindings.begin(), compute_bindings.end());
    }

    layouts.resize(4);
    for (auto& binding : shader_bindings)
    {
        layouts[binding.set_index].add_binding(binding);
        layouts[binding.set_index].set_index(binding.set_index);
    }

    uint32_t i = 0;
    while (i < layouts.size())
    {
        if (layouts[i].get_binding_count() == 0)
        {
            layouts.erase(layouts.begin() + i);
        }
        else
        {
            i++;
        }
    }

    for (auto& layout : layouts)
    {
        layout.build(device);
    }
}

// Shamelessly copied from SPIRV-Reflect sample :)
std::vector<v::Binding> ShaderText::collect_layouts(std::vector<uint32_t> &code)
{
    SpvReflectShaderModule module = {};
    SpvReflectResult result = spvReflectCreateShaderModule(
        code.size() * sizeof(uint32_t),
        code.data(),
        &module);

    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    //// Demonstrates how to generate all necessary data structures to create a
    //// VkDescriptorSetLayout for each descriptor set in this shader.
    //layouts.resize(sets.size());
    //layouts.insert(
    //    layouts.begin(),
    //    sets.size(),
    //    v::DescriptorLayout{}
    //);

    std::vector<v::Binding> shader_bindings;
    for (size_t set_index = 0; set_index < sets.size(); ++set_index)
    {
        const SpvReflectDescriptorSet& refl_set = *(sets[set_index]);
        for (uint32_t binding_index = 0; binding_index < refl_set.binding_count; ++binding_index)
        {
            VkDescriptorSetLayoutBinding binding{};
            const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[binding_index]);
            binding.binding = refl_binding.binding;
            binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
            binding.descriptorCount = 1;

            for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim)
            {
                binding.descriptorCount *= refl_binding.array.dims[i_dim];
            }
            binding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);

            // do we need shader name?
            //layout.shader_names.push_back(refl_binding.name);
            v::Binding shader_binding{};
            shader_binding.name = refl_binding.name;
            shader_binding.info = binding;
            shader_binding.set_index = refl_set.set;
            shader_bindings.push_back(shader_binding);
        }
    }

    spvReflectDestroyShaderModule(&module);

    return shader_bindings;

    /*
    // Log the descriptor set contents to stdout
    const char* t  = "  ";
    const char* tt = "    ";

    PrintModuleInfo(std::cout, module);
    std::cout << "\n\n";

    std::cout << "Descriptor sets:" << "\n";
    for (size_t index = 0; index < sets.size(); ++index) {
        auto p_set = sets[index];

        // descriptor sets can also be retrieved directly from the module, by set index
        auto p_set2 = spvReflectGetDescriptorSet(&module, p_set->set, &result);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        assert(p_set == p_set2);
        (void)p_set2;

        std::cout << t << index << ":" << "\n";
        PrintDescriptorSet(std::cout, *p_set, tt);
        std::cout << "\n\n";
    }
    */
}