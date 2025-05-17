import os
import re

# Block to centralize all target output to build/bin
CENTRAL_OUTPUT_SNIPPET = """
# Force all output binaries into the top-level build/bin directory
if(CMAKE_CONFIGURATION_TYPES)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
    endforeach()
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
"""

# Regex to remove any pre-existing OUTPUT_DIRECTORY lines
OUTPUT_PROP_REGEX = re.compile(
    r'^\s*set_target_properties\s*\(\s*\w+\s+PROPERTIES\s+((?:\w+_OUTPUT_DIRECTORY(_[A-Z]+)?\s+\S+\s*)+)\)', re.IGNORECASE)

def patch_cmake_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    modified = False
    new_lines = []
    already_patched = any("CMAKE_RUNTIME_OUTPUT_DIRECTORY" in line and "bin" in line for line in lines)

    for line in lines:
        if OUTPUT_PROP_REGEX.search(line):
            modified = True  # Remove line
            continue
        new_lines.append(line)

    if not already_patched:
        new_lines.append("\n" + CENTRAL_OUTPUT_SNIPPET.strip() + "\n")
        modified = True

    if modified:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        print(f"[PATCHED] {filepath}")

def patch_all_external():
    for root, _, files in os.walk("external"):
        for file in files:
            if file == "CMakeLists.txt":
                patch_cmake_file(os.path.join(root, file))

if __name__ == "__main__":
    patch_all_external()
