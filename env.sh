# Source this file in every new terminal before you use west:
#
#   source env.sh
#
# It activates the Python environment that `make setup` created, which is where west and Twister
# are installed, and tells Zephyr where it is and which compiler to use. native_sim programs are
# ordinary Linux programs, so the compiler is the host's own GCC, and the Zephyr SDK is not needed.

if [ -n "${ZSH_VERSION:-}" ]; then
    COURSE_ROOT="$(cd "$(dirname "${(%):-%x}")" && pwd)"
else
    COURSE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fi

if [ ! -f "$COURSE_ROOT/.venv/bin/activate" ] || [ ! -d "$COURSE_ROOT/deps/zephyr" ]; then
    echo "error: the workspace is not set up. Run 'make setup' in $COURSE_ROOT first." >&2
    return 1 2>/dev/null || exit 1
fi

source "$COURSE_ROOT/.venv/bin/activate"
export ZEPHYR_BASE="$COURSE_ROOT/deps/zephyr"
export ZEPHYR_TOOLCHAIN_VARIANT=host
