from pathlib import Path
import re

def patch_macro_guard(cmake_file: Path):
    lines = cmake_file.read_text().splitlines()
    output_lines = []
    in_macro = False
    patched = False

    for i, line in enumerate(lines):
        stripped = line.strip()

        # Detect the start of the macro
        if re.match(r"^macro\s*\(\s*vsg_add_cmake_support_files\s*\)", stripped):
            in_macro = True
            output_lines.append(line)
            continue

        # Inject guard immediately after macro declaration
        if in_macro and not patched:
            if "if(NOT VSG_ADD_CMAKE_SUPPORT_FILES)" not in lines[i + 1]:
                output_lines.append("    if(NOT VSG_ADD_CMAKE_SUPPORT_FILES)")
                output_lines.append("        return()")
                output_lines.append("    endif()")
                patched = True
            in_macro = False  # Only patch once
            output_lines.append(line)
            continue

        output_lines.append(line)

    cmake_file.write_text("\n".join(output_lines))
    print(f"✅ Patched: {cmake_file}")

def main():
    root = Path("external/VulkanSceneGraph")  # adjust path if needed
    for cmake_file in root.rglob("*Macros.cmake"):
        patch_macro_guard(cmake_file)

if __name__ == "__main__":
    main()
