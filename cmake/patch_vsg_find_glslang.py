import os
import sys
import re

if len(sys.argv) > 1:
    PROJECT_ROOT = os.path.abspath(sys.argv[1])
else:
    PROJECT_ROOT = os.getcwd()

vsg_cmake = os.path.join(PROJECT_ROOT, "external", "VulkanSceneGraph", "CMakeLists.txt")

if not os.path.exists(vsg_cmake):
    print(f"[ERROR] {vsg_cmake} not found.")
    sys.exit(1)

with open(vsg_cmake, "r", encoding="utf-8") as f:
    lines = f.readlines()

output = []
patched_once = False
patched_twice = False

for i, line in enumerate(lines):
    # Patch the fallback glslang 14 block
    if (
        "if (NOT glslang_FOUND)" in line
        and not patched_twice
        and not "AND" in line
    ):
        indent = re.match(r"^(\s*)", line).group(1)
        output.append(f"{indent}if (NOT glslang_FOUND AND NOT TARGET glslang)\n")
        patched_twice = True
    else:
        output.append(line)

with open(vsg_cmake, "w", encoding="utf-8") as f:
    f.writelines(output)

if patched_twice:
    print(f"[PATCHED] {vsg_cmake} (guarded fallback glslang find)")
else:
    print(f"[OK]      {vsg_cmake} (already guarded)")
