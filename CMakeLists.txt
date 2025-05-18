cmake_minimum_required(VERSION 3.7)

project(QMVision)

# set the use of C++17 globally as all examples require it
set(CMAKE_CXX_STANDARD 20)

set(VSG_ADD_CMAKE_SUPPORT_FILES OFF CACHE BOOL "" FORCE)
set(VSGXCHANGE_ADD_CMAKE_SUPPORT_FILES OFF CACHE BOOL "" FORCE)
set(VSGIMGUI_ADD_CMAKE_SUPPORT_FILES OFF CACHE BOOL "" FORCE)
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(VSG_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(VSG_INSTALL OFF CACHE BOOL "" FORCE)

if(MSVC)
    add_compile_options(/wd4251)
endif()

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
	
	# --- Patch VSG-related macro guards before configuring them ---
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E env PYTHONIOENCODING=utf-8
				${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/cmake/patch_vsg_macros.py
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE _patch_result
		OUTPUT_VARIABLE _patch_out
		ERROR_VARIABLE _patch_err
	)

	if(NOT _patch_result EQUAL 0)
		message(FATAL_ERROR "Failed to patch VSG macros:\n${_patch_out}\n${_patch_err}")
	else()
		message(STATUS "✅ VSG macros patched successfully:\n${_patch_out}")
	endif()
	
endif()

add_subdirectory(external/tracy)
add_subdirectory(external/assimp)
add_subdirectory(external/freetype)
add_library(Freetype::Freetype ALIAS freetype)
set(FREETYPE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/external/freetype/include")
set(FREETYPE_LIBRARY freetype)
set(Freetype_FOUND TRUE)

#add_subdirectory(external/SPIRV-Tools/SPIRV-Headers)
add_subdirectory(external/SPIRV-Tools)
add_subdirectory(external/glslang)

# --- Generate glslangConfig.cmake for VSG compatibility ---
set(GLSLANG_CONFIG_DIR "${CMAKE_BINARY_DIR}/fake_glslang_config/glslang")
file(MAKE_DIRECTORY "${GLSLANG_CONFIG_DIR}")

file(WRITE "${GLSLANG_CONFIG_DIR}/glslangConfig.cmake"
"
if(NOT TARGET glslang::glslang)
  add_library(glslang::glslang INTERFACE IMPORTED)
  set_target_properties(glslang::glslang PROPERTIES INTERFACE_LINK_LIBRARIES glslang)
endif()

if(NOT TARGET glslang::SPIRV)
  add_library(glslang::SPIRV INTERFACE IMPORTED)
  set_target_properties(glslang::SPIRV PROPERTIES INTERFACE_LINK_LIBRARIES SPIRV)
endif()
")

file(WRITE "${GLSLANG_CONFIG_DIR}/glslangConfigVersion.cmake"
"
set(PACKAGE_VERSION \"999.0.0\")

if(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  set(PACKAGE_VERSION_EXACT FALSE)
endif()
")

# --- Set glslang_DIR in cache so VSG can see it ---
set(glslang_DIR "${GLSLANG_CONFIG_DIR}" CACHE PATH "Expose glslang_DIR to subdirectories" FORCE)

# --- Diagnostics ---
message(STATUS ">>> glslang_DIR = ${glslang_DIR}")
file(GLOB check_glslang_config "${glslang_DIR}/glslangConfig.cmake")
message(STATUS ">>> Final sanity check for glslangConfig: ${check_glslang_config}")
find_package(glslang CONFIG REQUIRED)
message(STATUS "✅ Manual find_package(glslang) succeeded")

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