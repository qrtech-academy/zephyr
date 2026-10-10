# L04 Test Suite

Tests for everything [Appendix C](../../appendix/c_exercises.md) asks you to write. The suite is
described by [`suite.yaml`](./suite.yaml) and run by [`ci/suite.py`](../../../../ci/suite.py),
which says what every key means.

```bash
python3 ci/suite.py lectures/L04               # Against your code in ../
python3 ci/suite.py --solutions lectures/L04   # Against ../../appendix/solutions
```

`make test` at the repository root runs this suite and every other lecture's.

---

## What runs, and when

Each exercise set is an application of its own, and is tested once every file it asks for exists:

| Set | Tested when | Tests |
|---|---|---|
| `exercise1` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | The program's output, against [`exercise1.txt`](./exercise1.txt) |
| `exercise2` | `Kconfig`, `CMakeLists.txt`, `prj.conf`, `mutex.conf` and `src/main.c` exist | The output as built, against [`exercise2.txt`](./exercise2.txt), and built with `-DEXTRA_CONF_FILE=mutex.conf`, against [`exercise2-mutex.txt`](./exercise2-mutex.txt) |
| `exercise3` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | The program's output, against [`exercise3.txt`](./exercise3.txt) |
| `exercise4` | `CMakeLists.txt`, `prj.conf`, `src/door.h`, `src/door.c` and `src/main.c` exist | The program's output, against [`exercise4.txt`](./exercise4.txt); and the state machine itself, with the ztest application in [`door/`](./door) |

A set whose files do not all exist yet is reported as `SKIP`, with the first missing path, so a
file written under the wrong name is visible rather than silently untested.

**How a program is checked.** Each program is built for `native_sim/native/64` and run with
`-no-rt -stop_at=<seconds>`, so it runs in simulated time and stops by itself. Its output is
compared with the expected output line by line, after removing the colors, the
`*** Booting Zephyr OS build ... ***` banner, and the `Stopped at` line that `native_sim` prints at
the end. The timestamps are part of the output: they depend only on the priorities, start delays,
sleeps and busy waits the exercises specify, never on the host. A program that is still running
after 30 seconds of real time is reported as hung.

**How the state machine is checked.** [`door/`](./door) is a ztest application. The suite builds it
with `-DEXERCISE_DIR` pointing at your `exercise4`, and it compiles your `src/door.c`, with your
`src/door.h`, in with its tests, which enable `CONFIG_SMF` and `CONFIG_SMF_ANCESTOR_SUPPORT`
themselves. Each test starts with `door_init()` and drives the door with `door_handle()` through
sequences that `main.c`'s script does not contain: the button while opening, closing and stopped,
obstacles in every moving state, and end switches that do not apply. It passes if every test passes.
