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


#these handle the include directores for all my external dependencies, in case one these don't link properly, you can use set
#an absolute path from your root folder here
include_directories("${VULKAN_SDK}/Include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external_includes/glfw-3.3.4.bin.WIN64/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external_includes/glm")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external_includes/stb-master")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external_includes/assimp-5.0.1/include")


#engine composer

add_executable(main main.cpp)

target_link_libraries(main PRIVATE REQUIRED_LIBRARIES)
target_link_libraries(main PRIVATE "${VULKAN_SDK}/Lib/vulkan-1.lib")
target_link_libraries(main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external_includes/glfw-3.3.4.bin.WIN64/lib-vc2019/glfw3.lib")
target_link_libraries(main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external_includes/assimp-5.0.1/build/code/Debug/assimp-vc142-mtd.lib")