#include "config.hpp"

#include <iostream>

#include <fstream>

#include <filesystem>

#include <vector>

uint32_t hash_string(std::string str) {
    uint32_t hash = 5381;
    int c;

    const char* ch = str.c_str();

    while ((c = *ch++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

std::string goto_previous_directory(std::string file_path) {
#ifdef _WIN32
    size_t index = file_path.rfind("\\");
#else
    size_t index = file_path.rfind("/");
#endif

    std::string prev_dir = file_path.substr(0, index);

    return prev_dir;
}


std::string get_current_dir_name(std::string file_path) {
#ifdef _WIN32
    size_t index = file_path.rfind("\\");
#else
    size_t index = file_path.rfind("/");
#endif

    std::string current_dir = file_path.substr(index + 1, file_path.size());


    return current_dir;
}

std::string get_project_root(std::string file_path) {
    bool not_found = true;

    std::string current_path = file_path;
    while (not_found) {
        std::string dir_name = get_current_dir_name(current_path);
        if (dir_name.compare(PROJECT_ROOT) == 0) {
            not_found = false;
            break;
        }
        current_path = goto_previous_directory(current_path);
    }

    return current_path;
}


std::vector<char> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) { 
        std::cout << "could not find file at " << filename << std::endl; 
        throw std::runtime_error("could not open file \n");
    }

    size_t fileSize = (size_t)file.tellg();

    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

std::string get_extension_from_file_path(const std::string& filepath) {
    //get current file
    std::string filename = get_current_dir_name(filepath);
    
    //get location of "." seperating extension
    size_t index = filename.rfind(".");

    //get substring of extension from file name
    std::string extension = filename.substr(index+1, filename.size());

    return extension;
}

