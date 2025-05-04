cmake_minimum_required(VERSION 3.7)

project(QMVision)

# set the use of C++17 globally as all examples require it
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external/VulkanSceneGraph)
add_subdirectory(external/vsgXchange)

add_executable(QMVision QMVision.cpp)

target_link_libraries(QMVision PRIVATE vsg::vsg vsgXchange::vsgXchange)