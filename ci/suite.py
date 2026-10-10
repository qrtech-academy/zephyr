#!/usr/bin/env python3
"""Build and run one lecture's test suite.

A lecture's suite is described by lectures/LNN/exercises/test/suite.yaml, which maps each exercise
set to what the set asks you to write and how to check it:

    exercise1:
      needs: [exercise1/CMakeLists.txt, exercise1/prj.conf, exercise1/src/main.c]
      programs:
        - expected: exercise1.txt
          stop_at: 3
      ztest: exercise1

  needs       The files, relative to the workspace, that the set asks you to write. The set is
              tested once they all exist, and reported as SKIP until then.
  programs    Builds of your application, each built on its own and run one or more times. Every
              entry may give:
                app       The application directory, relative to the workspace. Defaults to the
                          set's name.
                cmake     Extra CMake arguments, e.g. -DEXTRA_CONF_FILE=verbose.conf. The
                          placeholder {test} is replaced by the absolute path of the test
                          directory, for files the suite provides. In a YAML [flow list], quote
                          an argument holding {test} or a comma, or YAML reads it as a mapping:
                          cmake: ["-DEXTRA_DTC_OVERLAY_FILE={test}/extra.overlay"]
                runs      A list of runs of the same build, one after another, in the same working
                          directory, so that files a run leaves behind (such as native_sim's
                          flash.bin) are there for the next. Each run takes the keys below. Without
                          `runs`, the entry itself is the one run, and takes them directly.
              A run may give:
                stop_at   Simulated seconds after which native_sim stops the program. A program
                          that returns from main() keeps running the idle thread, so every run
                          that does not end by itself needs one.
                args      Extra command-line arguments for the program, e.g. -seed=3.
                stdin     A file in the test directory to feed to the program's standard input,
                          for a program whose console or shell is on stdin and stdout. Without it,
                          standard input is empty.
                status    The exit status the program must end with. Defaults to 0.
                expected  A file in the test directory holding the output the program must print.
                          Without it the program only has to build, run and exit as expected.
                match     How the output is compared with `expected`: `exact` (the default), line
                          by line, or `contains`, where every expected line must be part of an
                          output line, in the same order, with any other lines in between.
  ztest       A directory in the test directory holding a ztest application, or a list of them.
              Each is built with -DEXERCISE_DIR set to the absolute path of the set's directory,
              compiles the sources it needs from there, and passes if every test in it passes.

The workspace is lectures/LNN/exercises, or lectures/LNN/appendix/solutions with --solutions.
Every program runs on native_sim/native/64, with simulated time decoupled from the host (-no-rt),
so a run of ten simulated seconds takes a few milliseconds and prints the same output every time.
The builds of a suite run in parallel; their results are reported in the order of suite.yaml.

Usage:
    ci/suite.py [--solutions] lectures/LNN
"""

import argparse
import difflib
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent
BOARD = "native_sim/native/64"

# How long a program may run, in host seconds, before it counts as hung. Simulated time runs far
# faster than that, so a program still running after this long is stuck in a busy loop.
RUN_TIMEOUT = 30

# How many applications to build at once. Every build runs a parallel Ninja of its own, so a few
# at a time is what keeps a machine responsive. SUITE_JOBS overrides it.
JOBS = int(os.environ.get("SUITE_JOBS", max(1, (os.cpu_count() or 4) // 4)))

# Lines that are not the program's own output: Zephyr's boot banner, which carries the Zephyr
# version, and the line native_sim prints when -stop_at ends the run.
BANNER = re.compile(r"^\*\*\* Booting Zephyr OS build .* \*\*\*$")
STOPPED = re.compile(r"^Stopped at [0-9.]+s$")
ANSI = re.compile(r"\x1b\[[0-9;]*m")

# The line AddressSanitizer or UndefinedBehaviorSanitizer starts an error report with.
SANITIZER = re.compile(r"ERROR: AddressSanitizer|runtime error: ")

# The line ztest prints as each test finishes, e.g. " PASS - test_rounds_up in 0.000 seconds".
RESULT = re.compile(r"^ (PASS|FAIL|SKIP) - (\S+) in ")


def normalize(output: str) -> list[str]:
    """Return the program's output as lines, without colors, the banner or the stop message."""
    lines = [ANSI.sub("", line).rstrip() for line in output.splitlines()]
    lines = [line for line in lines if not BANNER.match(line) and not STOPPED.match(line)]
    while lines and not lines[-1]:
        lines.pop()
    return lines


def lines_of(lines: list[str]) -> str:
    return f"{len(lines)} line" if len(lines) == 1 else f"{len(lines)} lines"


def first_errors(log: str, count: int = 6) -> list[str]:
    """Return the first compiler, linker, CMake, Kconfig or devicetree errors in a build log."""
    pattern = re.compile(r"\berror:|undefined reference|CMake Error|devicetree error|"
                         r"^warning: .* was assigned the value|user value .* ignored")
    lines = [ANSI.sub("", line) for line in log.splitlines()]
    return [line for line in lines if pattern.search(line)][:count]


def build(app: Path, build_dir: Path, cmake_args: list[str]) -> tuple[bool, str]:
    """Build an application for native_sim, returning whether it built and the build log."""
    command = ["west", "build", "-p", "auto", "-b", BOARD, "-d", str(build_dir), str(app)]
    if cmake_args:
        command += ["--", *cmake_args]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    log = result.stdout + result.stderr
    build_dir.mkdir(parents=True, exist_ok=True)
    (build_dir / "build.log").write_text(log)
    return result.returncode == 0, log


def run(executable: Path, args: list[str], cwd: Path, stdin: str | None) -> tuple[int | None, str]:
    """Run a native_sim program, returning its exit status (None if it hung) and its output."""
    try:
        result = subprocess.run(
            [str(executable), "-no-rt", *args],
            cwd=cwd,
            input=stdin if stdin is not None else "",
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=RUN_TIMEOUT,
            errors="replace",
        )
    except subprocess.TimeoutExpired as expired:
        output = expired.stdout or b""
        return None, output.decode(errors="replace") if isinstance(output, bytes) else output
    return result.returncode, result.stdout


def report_build_failure(name: str, what: str, log: str, build_dir: Path) -> None:
    print(f"FAIL  {name}: {what} does not build. The first errors:")
    for line in first_errors(log):
        print(f"      {line.strip()}")
    print(f"      The whole log is in {build_dir.relative_to(ROOT)}/build.log.")


def compare(label: str, spec: dict, lines: list[str], test_dir: Path) -> bool:
    """Compare a run's output with its expected output, as the run's `match` says."""
    expected = normalize((test_dir / spec["expected"]).read_text())

    if spec.get("match", "exact") == "contains":
        position = 0
        for wanted in expected:
            while position < len(lines) and wanted not in lines[position]:
                position += 1
            if position == len(lines):
                print(f"FAIL  {label}: the output never contains this line of {spec['expected']},")
                print("      after the lines before it:")
                print(f"      {wanted}")
                print("      The last lines of the output:")
                for line in lines[-8:]:
                    print(f"      {line}")
                return False
            position += 1
        print(f"PASS  {label}: prints the {lines_of(expected)} it should, in order.")
        return True

    if lines == expected:
        print(f"PASS  {label}: prints the {lines_of(expected)} it should.")
        return True

    print(f"FAIL  {label}: the output differs from {spec['expected']} ('-' expected, '+' yours):")
    diff = list(difflib.unified_diff(expected, lines, lineterm="", n=1))[2:]
    for line in diff[:20]:
        print(f"      {line}")
    if len(diff) > 20:
        print(f"      ... and {len(diff) - 20} more lines of difference.")
    return False


def check_run(label: str, spec: dict, build_dir: Path, work: Path, test_dir: Path) -> bool:
    """Run a built program once, and check how it ended and what it printed."""
    args = list(spec.get("args", []))
    if "stop_at" in spec:
        args.insert(0, f"-stop_at={spec['stop_at']}")
    stdin = (test_dir / spec["stdin"]).read_text() if "stdin" in spec else None

    status, output = run(build_dir / "zephyr" / "zephyr.exe", args, work, stdin)
    lines = normalize(output)
    if status is None:
        stop = f"stopped at {spec['stop_at']} s of simulated time" if "stop_at" in spec \
            else "ended by itself"
        print(f"FAIL  {label}: still running after {RUN_TIMEOUT} s of host time, although it should")
        print(f"      have {stop}. A busy loop? On native_sim, a loop that")
        print("      never calls into the kernel stops time, and with it everything else.")
        return False

    wanted = spec.get("status", 0)
    if status != wanted:
        # A sanitizer's report ends in pages of legend, so its first lines are the ones to show.
        first = next((i for i, line in enumerate(lines) if SANITIZER.search(line)), None)
        if first is not None:
            print(f"FAIL  {label}: exited with status {status}, not {wanted}. A sanitizer reported:")
            shown = lines[first:first + 4]
            shown += [line for line in lines[first + 4:] if line.startswith("SUMMARY:")]
        else:
            print(f"FAIL  {label}: exited with status {status}, not {wanted}. Its last lines:")
            shown = lines[-8:]
        for line in shown:
            print(f"      {line}")
        return False

    if "expected" not in spec:
        print(f"PASS  {label}: builds and runs.")
        return True
    return compare(label, spec, lines, test_dir)


def check_program(name: str, index: int, spec: dict, built: tuple[bool, str], build_dir: Path,
                  test_dir: Path) -> bool:
    """Run one of a set's built programs, as many times as the suite says."""
    label = f"{name}" if index == 0 else f"{name} (build {index + 1})"
    ok, log = built
    if not ok:
        report_build_failure(label, "your program", log, build_dir)
        return False

    # Every build's runs start in an empty directory, so that nothing a previous `make test` left
    # behind, such as a simulated flash, can change the result.
    work = build_dir / "run"
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir(parents=True)

    runs = spec.get("runs", [spec])
    passed = True
    for number, run_spec in enumerate(runs):
        run_label = label if len(runs) == 1 else f"{label}, run {number + 1}"
        if not check_run(run_label, run_spec, build_dir, work, test_dir):
            passed = False
            break
    return passed


def check_ztest(name: str, test_app: str, built: tuple[bool, str], build_dir: Path) -> bool:
    """Run a set's built ztest application."""
    label = f"{name} ({test_app})"
    ok, log = built
    if not ok:
        report_build_failure(label, "the test application, with your code,", log, build_dir)
        print("      Compare the names, types and signatures in the errors with the exercise.")
        return False

    status, output = run(build_dir / "zephyr" / "zephyr.exe", [], build_dir, None)
    lines = normalize(output)
    if status is None:
        print(f"FAIL  {label}: the tests were still running after {RUN_TIMEOUT} s. A busy loop, or")
        print("      a wait for something that never happens?")
        return False

    results = [RESULT.match(line) for line in lines]
    results = [match for match in results if match]
    failures = [match.group(2) for match in results if match.group(1) == "FAIL"]
    if status == 0 and results and not failures:
        print(f"PASS  {label}: {len(results)} test(s) passed.")
        return True

    if not failures:
        print(f"FAIL  {label}: exited with status {status} before its tests finished. Its last lines:")
        for line in lines[-8:]:
            print(f"      {line}")
        return False

    print(f"FAIL  {label}: {len(failures)} of {len(results)} test(s) failed: {', '.join(failures)}.")
    for index, line in enumerate(lines):
        if "Assertion failed" in line:
            print(f"      {line.strip()}")
            if index + 1 < len(lines) and not RESULT.match(lines[index + 1]):
                print(f"      {lines[index + 1].strip()}")
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("lecture", type=Path, help="the lecture directory, e.g. lectures/L01")
    parser.add_argument("--solutions", action="store_true",
                        help="test the reference solutions instead of your workspace")
    options = parser.parse_args()

    lecture = options.lecture.resolve()
    test_dir = lecture / "exercises" / "test"
    if options.solutions:
        work_dir = lecture / "appendix" / "solutions"
        variant = "solutions"
    else:
        work_dir = lecture / "exercises"
        variant = "exercises"
    build_root = ROOT / "build-ci" / "test" / lecture.name / variant

    suite = yaml.safe_load((test_dir / "suite.yaml").read_text()) or {}

    # Decide what to build: every program and ztest application of every set whose files exist.
    enabled = []
    builds = {}
    for name, spec in suite.items():
        needs = [work_dir / need for need in spec.get("needs", [])]
        missing = [need for need in needs if not need.exists()]
        if missing:
            enabled.append((name, spec, missing[0]))
            continue
        enabled.append((name, spec, None))
        for index, program in enumerate(spec.get("programs", [])):
            cmake = [arg.replace("{test}", str(test_dir)) for arg in program.get("cmake", [])]
            builds[(name, "program", index)] = (work_dir / program.get("app", name),
                                                build_root / f"{name}-program{index}", cmake)
        tests = spec.get("ztest", [])
        for test_app in [tests] if isinstance(tests, str) else tests:
            builds[(name, "ztest", test_app)] = (test_dir / test_app,
                                                 build_root / f"{name}-ztest-{test_app}",
                                                 [f"-DEXERCISE_DIR={work_dir / name}"])

    with ThreadPoolExecutor(max_workers=JOBS) as pool:
        futures = {key: pool.submit(build, *job) for key, job in builds.items()}
        results = {key: future.result() for key, future in futures.items()}

    # Run and report, in the order of suite.yaml.
    failed = False
    for name, spec, missing in enabled:
        if missing is not None:
            print(f"SKIP  {name}: {missing.relative_to(ROOT)} is not written yet.")
            continue

        print(f"TEST  {name}")
        for index, program in enumerate(spec.get("programs", [])):
            key = (name, "program", index)
            if not check_program(name, index, program, results[key], builds[key][1], test_dir):
                failed = True
        tests = spec.get("ztest", [])
        for test_app in [tests] if isinstance(tests, str) else tests:
            key = (name, "ztest", test_app)
            if not check_ztest(name, test_app, results[key], builds[key][1]):
                failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
