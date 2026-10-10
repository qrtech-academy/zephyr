# Create the workspace: a Python environment with west in .venv, and Zephyr in deps/.
setup:
	@bash ci/setup.sh

# Build every lecture example for native_sim.
build:
	@bash ci/build.sh

# Build every lecture example, run it, and check its output where its sample.yaml says what to expect.
examples:
	@bash ci/build.sh --run

# Build and run every lecture's test suite against your code in lectures/LNN/exercises.
# SOLUTIONS=1 runs the same tests against the reference solutions instead.
test:
	@SOLUTIONS=$(SOLUTIONS) bash ci/test.sh

# Format all C files (clang-format) in place.
format:
	@bash ci/format.sh

# Check formatting without modifying any files; fails if something isn't formatted.
format-check:
	@bash ci/format.sh --check

# Remove the output of `make build`, `make examples` and `make test`. The workspace stays.
clean:
	@rm -rf build-ci

.PHONY: setup build examples test format format-check clean
