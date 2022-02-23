#include "config.hpp"


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






