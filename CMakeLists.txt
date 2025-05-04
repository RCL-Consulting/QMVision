cmake_minimum_required(VERSION 3.7)

project(QMVision)

# set the use of C++17 globally as all examples require it
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external/assimp)
add_subdirectory(external/freetype)
add_library(Freetype::Freetype ALIAS freetype)
set(FREETYPE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include")
set(FREETYPE_LIBRARY freetype)
set(Freetype_FOUND TRUE)

# Allow glslang to use your local SPIRV-Tools
set(ALLOW_EXTERNAL_SPIRV_TOOLS ON CACHE BOOL "" FORCE)
set(SPIRV_TOOLS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/SPIRV-Tools)

add_subdirectory(${SPIRV_TOOLS_SOURCE_DIR})  # Build SPIRV-Tools first

# Now build glslang (which will use SPIRV-Tools)
add_subdirectory(external/glslang)

add_subdirectory(external/tracy)

set(VSG_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/VulkanSceneGraph)
add_subdirectory(external/vsgXchange)
add_subdirectory(external/vsgImGui)
add_library(vsg::vsgImGui ALIAS vsgImGui)

add_executable(QMVision QMVision.cpp)

target_link_libraries(QMVision PRIVATE vsg::vsg vsgXchange::vsgXchange vsg::vsgImGui assimp freetype glslang tracy ) 