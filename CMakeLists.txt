cmake_minimum_required(VERSION 3.16.3)
project(Antuco VERSION 1.0.0)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(CMAKE_CXX_STANDARD 20)

include_directories(inc/)
include_directories("C:/VulkanSDK/1.2.189.0/Include")
include_directories("C:/Users/Ganidhu/Documents/Visual Studio 2019/Libraries/glfw-3.3.4.bin.WIN64/include")
include_directories("C:/Users/Ganidhu/Documents/Visual Studio 2019/Libraries/glm")
include_directories("C:/Users/Ganidhu/Documents/Visual Studio 2019/Libraries/stb-master")
include_directories("C:/Users/Ganidhu/Documents/Visual Studio 2019/Libraries/assimp-5.0.1/include")

file(GLOB SOURCES lib/*.cpp)

#engine composer
add_library(
    required_libs
    ${SOURCES}
)

add_library(
    shaderc_combined
    SHARED
    IMPORTED
)

find_library(
    vulkan
    VULKAN_SDK
)

find_library(
    glfw
    GLFW
)

add_executable(main main.cpp)

target_link_libraries(main PRIVATE required_libs)
target_link_libraries(main PRIVATE "C:/VulkanSDK/1.2.189.0/Lib/vulkan-1.lib")
target_link_libraries(main PRIVATE "C:/Users/Ganidhu/Documents/Visual Studio 2019/Libraries/glfw-3.3.4.bin.WIN64/lib-vc2019/glfw3.lib")
target_link_libraries(main PRIVATE "C:/Users/Ganidhu/Documents/Visual Studio 2019/Libraries/assimp-5.0.1/build/code/Debug/assimp-vc142-mtd.lib")