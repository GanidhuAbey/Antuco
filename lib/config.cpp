#include "config.hpp"

#include <iostream>

#include <fstream>

#include <filesystem>

#include <vector>

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

VkCommandBuffer begin_command_buffer(VkDevice device, VkCommandPool command_pool) {
    //create command buffer
    VkCommandBuffer transferBuffer;

    VkCommandBufferAllocateInfo bufferAllocate{};
    bufferAllocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocate.commandPool = command_pool;
    bufferAllocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocate.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &bufferAllocate, &transferBuffer) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate memory for transfer buffer");
    }

    //begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(transferBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("one of the command buffers failed to begin");
    }

    return transferBuffer;
}


void end_command_buffer(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkCommandBuffer commandBuffer) {
    //end command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create succesfully end transfer buffer");
    };

    //destroy transfer buffer, shouldnt need it after copying the data.
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &commandBuffer);
}


