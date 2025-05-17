import os
import re

def patch_install_blocks(filepath):
    with open(filepath, "r", encoding="utf-8") as file:
        lines = file.readlines()

    patched_lines = []
    in_if_block = False
    block_lines = []
    block_modified = False

    for line in lines:
        stripped = line.strip()

        # Detect start of IF block
        if re.match(r"^\s*IF\s*\(", line, re.IGNORECASE):
            in_if_block = True
            block_lines = [line]
            block_modified = False
            continue

        # Track lines inside IF block
        if in_if_block:
            block_lines.append(line)
            if re.match(r"^\s*INSTALL\s*\(", line, re.IGNORECASE):
                block_modified = True
            if re.match(r"^\s*ENDIF\s*(\(|$)", line, re.IGNORECASE):
                # End of IF block
                if block_modified:
                    patched_lines.extend(f"# {l.rstrip()}  # stripped by script\n" for l in block_lines)
                    print(f"[PATCHED IF] {filepath}")
                else:
                    patched_lines.extend(block_lines)
                in_if_block = False
                block_lines = []
            continue

        # Outside IF block, just append line
        patched_lines.append(line)

    # Write only if modified
    with open(filepath, "w", encoding="utf-8") as file:
        file.writelines(patched_lines)

def patch_all_if_blocks(root_dir="external"):
    print("[INFO] Ensuring IF blocks are properly closed after stripping install() calls...")
    for subdir, _, files in os.walk(root_dir):
        for name in files:
            if name == "CMakeLists.txt":
                full_path = os.path.join(subdir, name)
                print(f"[DEBUG] Checking file: {full_path}")
                patch_install_blocks(full_path)

if __name__ == "__main__":
    patch_all_if_blocks()
