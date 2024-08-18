cmake_minimum_required(VERSION 3.1)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(PythonInterp 3.9 REQUIRED)

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

file(GLOB SOURCES lib/*.cpp lib/vulkan_wrapper/*.cpp lib/bedrock/*.cpp)

file(GLOB HEADERS inc/*.hpp inc/vulkan_wrapper/*.hpp inc/bedrock/*.hpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})


set(HEADER_FILES ${HEADERS})

set(SHADERC_SKIP_TESTS ON)

find_package(Vulkan)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glfw")

option(SPIRV_CROSS_CLI "Build the CLI binary. Requires SPIRV_CROSS_STATIC." OFF)
option(SPIRV_CROSS_ENABLE_TESTS "Enable SPIRV-Cross tests." OFF)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/SPIRV-Cross" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/shaderc" EXCLUDE_FROM_ALL)

option(FMT_UNICODE "Enable Unicode support." ON)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/fmt" EXCLUDE_FROM_ALL)

option(SPIRV_REFLECT_EXECUTABLE "" OFF)
option(SPIRV_REFLECT_STATIC_LIB "" ON)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/SPIRV-Reflect" EXCLUDE_FROM_ALL)


add_library(
    AntucoEngine
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(AntucoEngine PUBLIC "inc/")

# move external targets into external directory
set_target_properties(shaderc PROPERTIES FOLDER "External")
set_target_properties(glfw PROPERTIES FOLDER "External")
set_target_properties(fmt PROPERTIES FOLDER "External")
set_target_properties(spirv-reflect-static PROPERTIES FOLDER "External")
set_target_properties(spirv-cross-core PROPERTIES FOLDER "External")
set_target_properties(shaderc_util PROPERTIES FOLDER "External")
set_target_properties(core_tables PROPERTIES FOLDER "External")
set_target_properties(extinst_tables PROPERTIES FOLDER "External")
set_target_properties(enum_string_mapping PROPERTIES FOLDER "External")

include_directories("inc/")
include_directories("inc/components/skybox")
include_directories("${Vulkan_INCLUDE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/tinygltf")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glfw/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glm")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/stb")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/cimg")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/jpeg-9f")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/fmt/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/SPIRV-Reflect")

#compile assimp
add_executable(main main.cpp)

target_link_libraries(main PRIVATE AntucoEngine
                                   "${Vulkan_LIBRARIES}"
                                   glfw
                                   fmt::fmt
                                   shaderc
                                   spirv-reflect-static 
                                   spirv-cross-core
                                   "${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/jpeg.lib")
