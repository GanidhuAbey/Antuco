cmake_minimum_required(VERSION 3.1)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

find_package(PythonInterp 3.9.10 REQUIRED)

set(_change_MSVC_flags FALSE)
if(WIN32)
  set(MSVC TRUE)
  if(CMAKE_VERSION VERSION_LESS 3.15.0)
    set(_change_MSVC_flags TRUE)
  else()
    # Set MSVC runtime to MultiThreaded (/MT)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
endif()

project(Antuco VERSION 1.0.0)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

file(GLOB SOURCES lib/*.cpp lib/vulkan_wrapper/*.cpp lib/components/*.cpp lib/bedrock/*.cpp)

file(GLOB HEADERS inc/*.hpp)

set(HEADER_FILES ${HEADERS})

set(SHADERC_SKIP_TESTS ON)

find_package(Vulkan)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glfw")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/SPIRV-Cross")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/shaderc")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/crossguid")

add_library(
    REQUIRED_LIBRARIES
    ${SOURCES}
     "render_examples/bmw.hpp" "render_examples/bmw.cpp")

include_directories("inc/")
include_directories("inc/components/skybox")
include_directories("${Vulkan_INCLUDE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/tinygltf")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glfw/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glm")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/stb")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/crossguid/include")

#compile assimp
add_executable(main main.cpp "render_examples/bmw.hpp" "render_examples/bmw.cpp")

target_link_libraries(main PRIVATE REQUIRED_LIBRARIES 
                                   "${Vulkan_LIBRARIES}" 
                                   glfw 
                                   shaderc 
                                   spirv-cross-core
                                   crossguid)

