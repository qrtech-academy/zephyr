#!/usr/bin/env bash
#
# Set up the course workspace, inside the repository:
#
#   .venv/          A Python environment holding west, Twister and Zephyr's other Python tools.
#   .west/config    Makes the repository the west workspace, with west.yml as its manifest.
#   deps/zephyr     The Zephyr release that west.yml pins.
#
# Nothing is installed outside the repository, so deleting those three directories undoes it.
# Running the script again is safe: it brings the workspace up to date with west.yml.
#
# Usage:
#   ci/setup.sh
set -euo pipefail

# Absolute path to the repository root.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# The board every build in the course uses unless told otherwise. The 64-bit variant runs on any
# x86-64 Linux without the 32-bit libraries (gcc-multilib) that plain native_sim needs.
BOARD="native_sim/native/64"

################################################################################
# Terminate the script if a host tool the workspace needs is missing.
# Globals:
#   None
# Arguments:
#   None
################################################################################
check_host_tools() {
    local missing=()
    for tool in git cmake gcc g++ make python3; do
        command -v "$tool" &> /dev/null || missing+=("$tool")
    done
    if ! python3 -c "import venv, ensurepip" &> /dev/null; then
        missing+=("python3-venv")
    fi
    if [ ${#missing[@]} -ne 0 ]; then
        echo "error: missing ${missing[*]}. See lectures/L01/appendix/a_setup.md." >&2
        exit 1
    fi
}

# Install nothing until everything that cannot be installed from here is known to be present.
check_host_tools

# The Python environment, with west and ninja. Zephyr's requirements need Zephyr, so they come last.
if [ ! -x .venv/bin/python3 ]; then
    echo "==== Creating the Python environment in .venv"
    python3 -m venv .venv
fi
.venv/bin/pip install --quiet --upgrade pip
.venv/bin/pip install --quiet west ninja

# Make the repository the west workspace. `west init -l` would put the workspace in the parent
# directory instead, so the configuration is written directly.
if [ ! -f .west/config ]; then
    echo "==== Creating the west workspace"
    mkdir -p .west
    printf '[manifest]\npath = .\nfile = west.yml\n' > .west/config
fi
.venv/bin/west config zephyr.base deps/zephyr
.venv/bin/west config build.board "$BOARD"

echo "==== Fetching Zephyr (the first time takes a few minutes)"
.venv/bin/west update

echo "==== Installing Zephyr's Python requirements"
.venv/bin/pip install --quiet \
    -r deps/zephyr/scripts/requirements-base.txt \
    -r deps/zephyr/scripts/requirements-build-test.txt \
    -r deps/zephyr/scripts/requirements-run-test.txt

echo
echo "Setup complete. In every new terminal, run 'source env.sh' before using west."
