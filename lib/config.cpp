#include "config.hpp"


std::string goto_previous_directory(std::string file_path) {
    size_t index = file_path.rfind("/");
    
    std::string prev_dir = file_path.substr(0, index);

    return prev_dir;
}
