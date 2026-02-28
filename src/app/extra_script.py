Import("env")
import os, sys

# --- Locate MinGW-w64 ---
# Check in platform/packages/mingw64 (project-local), then fall back to PATH
project_dir = env.subst("$PROJECT_DIR")
local_mingw = os.path.join(project_dir, "platform", "packages", "mingw64", "bin")

if os.path.isdir(local_mingw):
    env.PrependENVPath("PATH", local_mingw)
    print("Using local MinGW-w64: " + local_mingw)
