cmake_minimum_required(VERSION 3.7)

project(QMVision)

# set the use of C++17 globally as all examples require it
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external/VulkanSceneGraph)
add_subdirectory(external/vsgXchange)
add_subdirectory(external/assimp)
add_subdirectory(external/freetype)
add_subdirectory(external/glslang)
add_subdirectory(external/imgui)
add_subdirectory(external/tracy)
add_subdirectory(external/vsgImGui)

add_executable(QMVision QMVision.cpp)

target_link_libraries(QMVision PRIVATE vsg::vsg vsgXchange::vsgXchange vsg::vsgImGui assimp freetype glslang imgui tracy ) 