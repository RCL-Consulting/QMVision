cmake_minimum_required(VERSION 3.7)

project(QMVision)

# set the use of C++17 globally as all examples require it
set(CMAKE_CXX_STANDARD 20)

set(VSG_BUILD_INSTALL OFF CACHE BOOL "Disable install export targets in VSG" FORCE)
option(FETCH_DEPENDENCIES "Automatically fetch vendored dependencies" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)    # For .exe/.dll
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)    # For .dll/.so
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)    # For .lib/.a

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
endforeach()

if(FETCH_DEPENDENCIES)
	# Run Python script to clone vendor dependencies if needed
	find_program(PYTHON_EXECUTABLE python)

	set(VENDOR_CHECK
		"${CMAKE_SOURCE_DIR}/external/VulkanSceneGraph"
		"${CMAKE_SOURCE_DIR}/external/assimp"
		"${CMAKE_SOURCE_DIR}/external/freetype"
		"${CMAKE_SOURCE_DIR}/external/glslang"
		"${CMAKE_SOURCE_DIR}/external/SPIRV-Tools"
		"${CMAKE_SOURCE_DIR}/external/vsgImGui"
		"${CMAKE_SOURCE_DIR}/external/vsgXchange"
		"${CMAKE_SOURCE_DIR}/external/tracy"
		"${CMAKE_SOURCE_DIR}/external/QuadMind"
	)

	set(VENDORS_MISSING FALSE)
	foreach(path ${VENDOR_CHECK})
		if(NOT EXISTS "${path}")
			set(VENDORS_MISSING TRUE)
		endif()
	endforeach()

	if(VENDORS_MISSING)
		message(STATUS "Missing vendor directories detected. Running clone_vendors.py...")
		execute_process(
			COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/cmake/clone_vendors.py ${CMAKE_SOURCE_DIR}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			RESULT_VARIABLE CLONE_RESULT
		)
		if(NOT CLONE_RESULT EQUAL 0)
			message(FATAL_ERROR "Vendor clone script failed with code ${CLONE_RESULT}")
		endif()
	endif()
	
endif()

add_subdirectory(external/tracy)
add_subdirectory(external/assimp)
add_subdirectory(external/freetype)
add_library(Freetype::Freetype ALIAS freetype)
set(FREETYPE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include")
set(FREETYPE_LIBRARY freetype)
set(Freetype_FOUND TRUE)

# Allow glslang to use your local SPIRV-Tools
set(ALLOW_EXTERNAL_SPIRV_TOOLS ON CACHE BOOL "" FORCE)
set(SPIRV_TOOLS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/SPIRV-Tools)
if (MSVC)
    add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    add_compile_options(/wd4251)  # Suppress "needs to have dll-interface"
endif()

add_subdirectory(${SPIRV_TOOLS_SOURCE_DIR})  # Build SPIRV-Tools first

# Now build glslang (which will use SPIRV-Tools)
add_subdirectory(external/glslang)
# Aliases must exist *before* VSG is added
if(TARGET glslang)
    add_library(glslang::glslang ALIAS glslang)
endif()

if(TARGET SPIRV)
    add_library(glslang::SPIRV ALIAS SPIRV)
endif()

if(TARGET glslang-default-resource-limits)
    add_library(glslang::glslang-default-resource-limits ALIAS glslang-default-resource-limits)
endif()

# Declare glslang_FOUND before VSG uses it
set(glslang_FOUND TRUE CACHE BOOL "" FORCE)

set(VSG_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(VSG_SUPPORTS_ShaderCompiler ON CACHE BOOL "Enable shader compiler support" FORCE)

if (TARGET glslang)
    message(STATUS "TARGET glslang exists ✅")
else()
    message(WARNING "TARGET glslang does NOT exist ❌")
endif()

if (TARGET glslang::glslang)
    message(STATUS "ALIAS glslang::glslang exists ✅")
else()
    message(WARNING "ALIAS glslang::glslang does NOT exist ❌")
endif()

add_subdirectory(external/VulkanSceneGraph)

# Defer until vsg target is available
add_custom_target(copy_vsg_dll ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:vsg>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    DEPENDS vsg
    COMMENT "Copying vsg.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

add_subdirectory(external/vsgXchange)
add_subdirectory(external/vsgImGui)
add_library(vsg::vsgImGui ALIAS vsgImGui)
add_subdirectory(external/QuadMind/QMorphLib)

add_executable(QMVision QMVision.cpp)

target_include_directories(QMVision PRIVATE external/QuadMind/QMorphLib)
target_link_libraries(QMVision PRIVATE vsg::vsg vsgXchange::vsgXchange vsg::vsgImGui assimp freetype glslang TracyClient QMorphLib) 