# PlatformIO pre-build script (attached to every environment via
# env:common): picks which sources src/CMakeLists.txt builds.
#
# src/CMakeLists.txt chooses between the normal Urchin app and the isolated
# hardware bench tests based on an environment variable. Relying on people
# to set that by hand in their shell was a reliability trap:
#   - forget it once and PlatformIO silently builds (and flashes) the full
#     Urchin app instead of the test, so the hardware just never responds;
#   - leave it set and the next "normal" build flashes the test instead;
#   - PlatformIO only re-runs CMake when a CMakeLists/sdkconfig file
#     changes, NOT when an environment variable changes, so a build dir
#     configured the wrong way stays wrong even after fixing the variable.
# This script sets exactly the right variable for the environment being
# built, clears the others, and forces a CMake re-configure whenever a
# build dir was last configured for a different selection.

import os

Import("env")  # noqa: F821 -- injected by PlatformIO/SCons

# PlatformIO environment -> variable src/CMakeLists.txt checks.
# Environments not listed here build the normal Urchin app.
BENCH_ENVS = {
    "DBoxEcho": "DBOX_ECHO_ONLY",
    "HerkulexTest": "HERKULEX_TEST_ONLY",
    "DBoxMotor": "DBOX_MOTOR_ONLY",
}

pioenv = env["PIOENV"]  # noqa: F821
selected = BENCH_ENVS.get(pioenv)

for var in BENCH_ENVS.values():
    os.environ.pop(var, None)
if selected:
    os.environ[selected] = "1"

build_dir = env.subst("$BUILD_DIR")  # noqa: F821
stamp = os.path.join(build_dir, "source_selection.stamp")
cmake_cache = os.path.join(build_dir, "CMakeCache.txt")
wanted = selected or "URCHIN_APP"

previous = None
if os.path.isfile(stamp):
    with open(stamp) as f:
        previous = f.read().strip()

if previous != wanted:
    if os.path.isfile(cmake_cache):
        print("select_sources: %s was configured for %s; forcing CMake re-configure for %s"
              % (pioenv, previous or "an unknown selection", wanted))
        os.remove(cmake_cache)
    os.makedirs(build_dir, exist_ok=True)
    with open(stamp, "w") as f:
        f.write(wanted + "\n")
