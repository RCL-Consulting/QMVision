import os
import re

# Patterns to match beginning of blocks
install_command_re = re.compile(r"^\s*install\s*\(", re.IGNORECASE)
export_command_re = re.compile(r"^\s*export\s*\(", re.IGNORECASE)
cmake_support_macro_re = re.compile(r"^\s*vsg_add_cmake_support_files\s*\(", re.IGNORECASE)

def strip_install_commands_from_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    output = []
    inside_block = False
    paren_depth = 0
    modified = False

    for line in lines:
        stripped = line.strip()

        # Detect start of any known problematic block
        if (install_command_re.match(stripped) or
            export_command_re.match(stripped) or
            cmake_support_macro_re.match(stripped)):
            inside_block = True
            paren_depth = line.count("(") - line.count(")")
            output.append(f"# {line.rstrip()}  # stripped by script\n")
            modified = True
            continue

        if inside_block:
            paren_depth += line.count("(") - line.count(")")
            output.append(f"# {line.rstrip()}  # stripped by script\n")
            if paren_depth <= 0:
                inside_block = False
            continue

        output.append(line)

    if modified:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.writelines(output)
        print(f"[PATCHED] {file_path}")
    else:
        print(f"[OK]      {file_path}")

def walk_and_strip_cmake(root_dir):
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename == "CMakeLists.txt":
                full_path = os.path.join(dirpath, filename)
                strip_install_commands_from_file(full_path)

# Call this with the path to your vendor root
walk_and_strip_cmake("external")
