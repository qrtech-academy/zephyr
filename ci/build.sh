#!/usr/bin/env bash
#
# Build every lecture example, and with --run also run it, through Twister.
#
# Every example is a Zephyr application with a sample.yaml (or, for a test application, a
# testcase.yaml) beside its CMakeLists.txt, exactly like the samples in Zephyr's own tree. That file
# is what Twister looks for: it says which board the example runs on and, where the example prints
# something worth checking, which lines its output must contain. Building and checking the examples
# is therefore the same command that Zephyr's CI runs over Zephyr's samples.
#
# Usage:
#   ci/build.sh          build every example
#   ci/build.sh --run    build every example, run it, and check its output
set -euo pipefail
shopt -s nullglob

# Absolute path to the repository root.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
source env.sh

roots=()
for dir in lectures/*/examples; do
    roots+=(-T "$dir")
done

mode=(--build-only)
if [ "${1:-}" == "--run" ]; then
    mode=()
fi

# Twister runs the builds in parallel and writes everything under build-ci/twister. Its summary
# names every example that failed, and --inline-logs prints the build or run log of each one.
west twister "${roots[@]}" -p native_sim/native/64 -O build-ci/twister --inline-logs \
    "${mode[@]}"
