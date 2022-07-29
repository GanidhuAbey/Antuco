#include "shader_text.hpp"

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

    compiled_code = compile_file_to_spirv("shader", shader_kind, string_code);    
}

ShaderText::~ShaderText() {

}

std::vector<uint32_t> ShaderText::get_code() {
    return compiled_code;
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

  return {module.cbegin(), module.cend()};}

