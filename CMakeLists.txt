#Just in case something doesn't link properly and an error shows up i tried to make this as easy as i could to edit

#If you are getting an error with vulkan not linking properly, its most likely to do with VULKAN_SDK failing to find the path to your vulkan sdk
#try writing out the path of your vulkan location fully instead of using VULKAN_SDK and see if that fixes it

cmake_minimum_required(VERSION 3.1)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_STANDARD 17)

project(Antuco VERSION 1.0.0)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

include_directories(inc/)



#finds all my internal dependencies and adds it to a single variable for linking, if needed you can add the file path of each individual .cpp file being linked here
file(GLOB SOURCES lib/*.cpp)

add_library(
    REQUIRED_LIBRARIES
    ${SOURCES}
     "render_examples/bmw.hpp" "render_examples/bmw.cpp")

find_package(Vulkan)
find_package(Vulkan)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/assimp" "${CMAKE_CURRENT_SOURCE_DIR}/external/include/assimp")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glfw")
#these handle the include directores for all my external dependencies, in case one these don't link properly, you can use set
#an absolute path from your root folder here
include_directories("${Vulkan_INCLUDE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glfw/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glm")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/stb")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/assimp/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/build/assimp/include")


#engine composer

#how to handle this part...


#compile assimp
add_executable(main main.cpp "render_examples/bmw.hpp" "render_examples/bmw.cpp")

target_link_libraries(main PRIVATE REQUIRED_LIBRARIES "${Vulkan_LIBRARIES}" assimp::assimp glfw)
