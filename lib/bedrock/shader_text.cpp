#include <bedrock/shader_text.hpp>

#include "logger/interface.hpp"
#include "config.hpp"
#include <shaderc/shaderc.hpp>

#include <iostream>
#include <string>

#define VERTEX "VERTEX"
#define FRAGMENT "FRAGMENT"


using namespace br;

ShaderText::ShaderText(std::string shader_code_path, ShaderKind kind) {
    auto code = read_file(shader_code_path);
    std::string string_code(code.begin(), code.end());
    
    shaderc_shader_kind shader_kind;
    m_kind = kind;
    switch(kind) {
        case ShaderKind::VERTEX_SHADER:
            shader_kind = shaderc_vertex_shader;
            m_define = VERTEX;
            break;
        case ShaderKind::FRAGMENT_SHADER:
            shader_kind = shaderc_fragment_shader;
            m_define = FRAGMENT;
            break;
        case ShaderKind::COMPUTE_SHADER:
            shader_kind = shaderc_compute_shader;
            break;
        default:
            break;
    }

    m_compiled_code = compile_file_to_spirv(
        get_current_file_name(shader_code_path), 
        shader_kind, 
        string_code
    );

    create_layout_data();
}

ShaderText::~ShaderText() {

}

std::vector<uint32_t>& ShaderText::get_code() {
    return m_compiled_code;
}

// Check file type
bool is_hlsl(const std::string& source_name) {
    printf("name : %s \n", source_name.c_str());    
    if (get_extension_from_file_path(source_name) == "hlsl") {
        return true;
    }
    return false;
}

void ShaderText::build_layouts(v::Device& device) {
    assert(!m_layouts_built);
    
    mp_layouts = std::make_unique<v::BindingLayouts>(device);
    mp_layouts->initialize_layouts(get_layout_data());

    m_layouts_built = true;
}

VkDescriptorSetLayout& ShaderText::get_layout(uint32_t index) {
    assert(m_layouts_built && index < m_layout_data.size());
    return mp_layouts->get_layout(index);
}

std::vector<uint32_t> ShaderText::compile_file_to_spirv(const std::string& source_name, 
    shaderc_shader_kind kind,
    const std::string& source,
    bool optimize) 
{   
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    // Combine shaders using defines to seperate vertex and fragment
    // Like -DMY_DEFINE=1
    options.AddMacroDefinition(m_define.c_str(), "1");
    if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

    if (is_hlsl(source_name)) {
        options.SetSourceLanguage(shaderc_source_language_hlsl);
    }

    shaderc::SpvCompilationResult module =
        compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);
        

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << module.GetErrorMessage();
        return std::vector<uint32_t>();
    }

    return {module.cbegin(), module.cend()};
}

// Shamelessly copied from SPIRV-Reflect sample :)
void ShaderText::create_layout_data() {
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

            layout.shader_names.push_back(refl_binding.name);
        }
        layout.set_number = refl_set.set;
        layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout.create_info.bindingCount = refl_set.binding_count;
        layout.create_info.pBindings = layout.bindings.data();
    }
    
    spvReflectDestroyShaderModule(&module);
    
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
