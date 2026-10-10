#!/usr/bin/env bash
#
# Build and run every lecture's test suite.
#
# Each suite tests the code in its lecture's workspace, lectures/LNN/exercises. An exercise set is
# built only once the files it asks for exist, and reported as SKIP until then, so a fresh clone
# passes. Write a set's files and its tests switch themselves on, with no list to edit anywhere.
#
# With SOLUTIONS=1, every suite tests its lecture's reference solutions instead. That is how CI
# proves that every test in the course passes against a correct answer.
#
# Usage:
#   ci/test.sh                 test your workspace
#   SOLUTIONS=1 ci/test.sh     test the reference solutions
set -euo pipefail
shopt -s nullglob

# Absolute path to the repository root.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
source env.sh

args=()
if [ "${SOLUTIONS:-0}" == "1" ]; then
    args+=(--solutions)
fi

suites=0
failed=()

for suite in lectures/*/exercises/test/suite.yaml; do
    lecture="$(dirname "$(dirname "$(dirname "$suite")")")"
    echo "==== ${lecture}"
    # Run every suite even if an earlier one failed, so one report shows everything.
    if ! python3 ci/suite.py "${args[@]}" "$lecture"; then
        failed+=("$lecture")
    fi
    suites=$((suites + 1))
    echo
done

if [ ${#failed[@]} -ne 0 ]; then
    echo "Test: ${#failed[@]} of ${suites} suite(s) failed: ${failed[*]}"
    exit 1
fi
echo "Test: ${suites} suite(s) passed."
