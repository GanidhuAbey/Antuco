#Just in case something doesn't link properly and an error shows up i tried to make this as easy as i could to edit

#If you are getting an error with vulkan not linking properly, its most likely to do with VULKAN_SDK failing to find the path to your vulkan sdk
#try writing out the path of your vulkan location fully instead of using VULKAN_SDK and see if that fixes it

cmake_minimum_required(VERSION 3.16.3)
project(Antuco VERSION 1.0.0)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(CMAKE_CXX_STANDARD 20)

include_directories(inc/)

#how to handle this part...



#finds all my internal dependencies and adds it to a single variable for linking, if needed you can add the file path of each individual .cpp file being linked here
file(GLOB SOURCES lib/*.cpp)

add_library(
    REQUIRED_LIBRARIES
    ${SOURCES}
)

find_package(Vulkan)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/include/assimp" "${CMAKE_CURRENT_SOURCE_DIR}/external/build/assimp")

#these handle the include directores for all my external dependencies, in case one these don't link properly, you can use set
#an absolute path from your root folder here
include_directories("${Vulkan_INCLUDE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glfw/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/glm")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/stb")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/include/assimp/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/build/assimp/include")


#engine composer

#compile assimp
#add_library(assimp_dll "${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/assimp-vc142-mtd.dll")
add_executable(main main.cpp)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/assimp-vc142-mtd.dll" "${CMAKE_CURRENT_BINARY_DIR}/assimp-vc142-mtd.dll" COPYONLY)

target_link_libraries(main PRIVATE REQUIRED_LIBRARIES)
target_link_libraries(main PRIVATE "${Vulkan_LIBRARIES}")
target_link_libraries(main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/glfw3.lib")
target_link_libraries(main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/libraries/assimp-vc142-mtd.lib")