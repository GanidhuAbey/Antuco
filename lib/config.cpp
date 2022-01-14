#include "config.hpp"


std::string goto_previous_directory(std::string file_path) {
    size_t index = file_path.rfind("/");
    printf("index: %s, position: %d \n", file_path.c_str(), index);
    
    std::string prev_dir = file_path.substr(0, index);

    return prev_dir;
}
