/* ------------ config.hpp ------------------
 * Handle api-agnostic render config settings
 * ------------------------------------------
*/

#pragma once

#include <cstdint>
#include <vector>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if defined(TARGET_OS_MAC) && defined(TARGET_CPU_ARM)
#define APPLE_M1
#endif
#endif


#include <string>

const uint32_t MAX_SHADOW_CASTERS = 4;
const char PROJECT_ROOT[7] = "Antuco";


//go to the previous directory given a file path
//ex "Users/user1/project" -> "Users/user1"
extern std::string goto_previous_directory(std::string file_path);

//REQUIRES: string must be a file path, string cannot have "/" or "\" at the end.
//EFFECTS: returns the name the current dir/file
//ex "Users/user1/project/file.cpp" -> "file.cpp"
//ex "Users\user1\Antuco" -> "Antuco"
extern std::string get_current_dir_name(std::string);


//REQUIRES: string must be file path, be within file/dir within PROOJECT_ROOT
//EFFECTS: returns the absolute path to root project folder
//ex "Users/user1/{PROJECT_ROOT}/some_dir/file.cpp" -> "Users/user1/{PROJECT_ROOT}"
extern std::string get_project_root(std::string);

extern std::string get_file_name(const std::string& path);


//REQUIRES: filename must refer to text file within PROJECT_ROOT
//EFFECTS: returns the contents within file
extern std::vector<char> read_file(const std::string& filename);

//REQUIRES: filename should be a string representing a path to a file
//EFFECTS: returns the extension of the file given
extern std::string get_extension_from_file_path(const std::string& filename);
