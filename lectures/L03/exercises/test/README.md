# L03 Test Suite

Tests for everything [Appendix B](../../appendix/b_exercises.md) asks you to write. The suite is
described by [`suite.yaml`](./suite.yaml) and run by [`ci/suite.py`](../../../../ci/suite.py),
which says what every key means.

```bash
python3 ci/suite.py lectures/L03               # Against your code in ../
python3 ci/suite.py --solutions lectures/L03   # Against ../../appendix/solutions
```

`make test` at the repository root runs this suite and every other lecture's.

---

## What runs, and when

Each exercise set is an application of its own, and is tested once every file it asks for exists:

| Set | Tested when | Tests |
|---|---|---|
| `exercise1` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | The trace of Exercise 1.3, against [`exercise1.txt`](./exercise1.txt) |
| `exercise2` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | The samples, contexts and return values of Exercise 2.2, against [`exercise2.txt`](./exercise2.txt) |
| `exercise3` | `CMakeLists.txt`, `prj.conf`, `app.overlay` and `src/main.c` exist | The debounced presses of Exercise 3.2, against [`exercise3.txt`](./exercise3.txt) |
| `exercise4` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | The conversions of Exercise 4.2 or 4.3, against [`exercise4.txt`](./exercise4.txt) |

Exercise Set 5 has no code, and no tests. A set whose files do not all exist yet is reported as
`SKIP`, with the first missing path, so a file written under the wrong name is visible rather than
silently untested.

**How a program is checked.** Each program is built for `native_sim/native/64` and run with
`-no-rt -stop_at=<seconds>`, so it runs in simulated time and stops by itself. Its output is
compared with the expected output line by line, after removing the colors, the
`*** Booting Zephyr OS build ... ***` banner, and the `Stopped at` line that `native_sim` prints at
the end.

**Every line of these traces depends on time and on the scheduler**, and on `native_sim` both are
exact: a thread that wakes 10 ms late, or runs before a thread of higher priority, shows up as a
difference. Read the diff the suite prints line by line, with Appendix A open.

**A busy loop is reported as a hang.** A program that is still running after 30 seconds of real
time, although it should have stopped after a simulated second or two, is killed and reported as
probably stuck in a loop that never calls into the kernel. That is what the program in
Exercise 4.1 does.
