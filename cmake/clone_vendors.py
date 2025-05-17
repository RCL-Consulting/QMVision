import os
import subprocess
import sys

# Project root path
if len(sys.argv) > 1:
    PROJECT_ROOT = os.path.abspath(sys.argv[1])
else:
    PROJECT_ROOT = os.getcwd()

EXTERNAL_DIR = os.path.join(PROJECT_ROOT, "external")

# Main repos to clone under external/
REPOS = {
    "VulkanSceneGraph": "https://github.com/vsg-dev/VulkanSceneGraph.git",
    "assimp":           "https://github.com/assimp/assimp.git",
    "freetype":         "https://github.com/freetype/freetype.git",
    "glslang":          "https://github.com/KhronosGroup/glslang.git",
    "SPIRV-Tools":      "https://github.com/KhronosGroup/SPIRV-Tools.git",
    "vsgImGui":         "https://github.com/vsg-dev/vsgImGui.git",
    "vsgXchange":       "https://github.com/vsg-dev/vsgXchange.git",
    "tracy":            "https://github.com/wolfpld/tracy.git",
    "QuadMind":         "https://github.com/RCL-Consulting/QuadMind.git",
}

# Special case for SPIRV-Headers
SPIRV_HEADERS_REPO = "https://github.com/KhronosGroup/SPIRV-Headers.git"
SPIRV_HEADERS_DEST = os.path.join(EXTERNAL_DIR, "SPIRV-Tools", "external", "SPIRV-Headers")

def clone_repo(name, url):
    dest = os.path.join(EXTERNAL_DIR, name)
    if os.path.isdir(dest):
        print(f"[SKIP] {name} already exists.")
        return

    print(f"[CLONE] {name} -> {url}")
    subprocess.run(["git", "clone", "--recursive", url, dest], check=True)

def clone_spirv_headers():
    if os.path.isdir(SPIRV_HEADERS_DEST):
        print(f"[SKIP] SPIRV-Headers already exists at expected path.")
        return

    print(f"[CLONE] SPIRV-Headers -> {SPIRV_HEADERS_REPO}")
    os.makedirs(os.path.dirname(SPIRV_HEADERS_DEST), exist_ok=True)
    subprocess.run(["git", "clone", "--recursive", SPIRV_HEADERS_REPO, SPIRV_HEADERS_DEST], check=True)

def main():
    try:
        os.makedirs(EXTERNAL_DIR, exist_ok=True)
    except Exception as e:
        print(f"[ERROR] Could not create 'external' folder: {e}")
        return 1

    for name, url in REPOS.items():
        try:
            clone_repo(name, url)
        except subprocess.CalledProcessError:
            print(f"[ERROR] Failed to clone {name}")
            return 1

    try:
        clone_spirv_headers()
    except subprocess.CalledProcessError:
        print(f"[ERROR] Failed to clone SPIRV-Headers")
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main())