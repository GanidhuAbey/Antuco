// Handle api-agnostic render config settings

#pragma once

#include <cstdint>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if defined(TARGET_OS_MAC) && defined(TARGET_CPU_ARM)
#define APPLE_M1
#endif
#endif


#include <string>

const uint32_t MAX_SHADOW_CASTERS = 4;


//go to the previous directory given a file path
//ex "Users/user1/project" -> "Users/user1"
extern std::string goto_previous_directory(std::string file_path);
