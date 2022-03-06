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

    compiled_code = compile_file_to_assembly("shader", shader_kind, string_code);    
}

ShaderText::~ShaderText() {

}

std::string ShaderText::get_code() {
    return compiled_code;
}

std::string ShaderText::compile_file_to_assembly(const std::string& source_name,
                                     shaderc_shader_kind kind,
                                     const std::string& source,
                                     bool optimize) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  // Like -DMY_DEFINE=1
  options.AddMacroDefinition("MY_DEFINE", "1");
  if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

  shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(
      source, kind, source_name.c_str(), options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    std::cerr << result.GetErrorMessage();
    return "";
  }

  return {result.cbegin(), result.cend()};
}

