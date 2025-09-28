cmake_minimum_required(VERSION 3.23)
project(QMVision LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg packages
find_package(wxWidgets CONFIG REQUIRED COMPONENTS core base gl)
find_package(glad CONFIG REQUIRED)
find_package(OpenGL REQUIRED)  # OpenGL::GL

# If you want to pull QMorphLib from Git:
include(FetchContent)  # <-- REQUIRED for FetchContent_*
FetchContent_Declare(
  QMorphLib
  GIT_REPOSITORY https://github.com/RCL-Consulting/QuadMind.git
  GIT_TAG main
)
FetchContent_MakeAvailable(QMorphLib)

add_executable(QMVision WIN32
  src/main.cpp
  src/MainFrame.cpp src/MainFrame.h
  src/GLCanvas.cpp  src/GLCanvas.h
  src/gl/Shader.h src/gl/Math.h  
  src/gl/GpuMesh.h src/gl/Picker.h  "src/gl/PSLGOverlay.h" "src/gl/PSLGOverlay.cpp")

target_link_libraries(QMVision PRIVATE
  wx::core wx::base wx::gl
  glad::glad
  OpenGL::GL
  QMorphLib            # remove if you didn't FetchContent it
)

if(MSVC)
  target_compile_options(QMVision PRIVATE /W4 /permissive- /Zc:preprocessor)
  set_property(TARGET QMVision PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
