# L01 Test Suite

Tests for everything [Appendix C](../../appendix/c_exercises.md) asks you to write. The suite is
described by [`suite.yaml`](./suite.yaml) and run by [`ci/suite.py`](../../../../ci/suite.py),
which says what every key means.

```bash
python3 ci/suite.py lectures/L01               # Against your code in ../
python3 ci/suite.py --solutions lectures/L01   # Against ../../appendix/solutions
```

`make test` at the repository root runs this suite and every other lecture's.

---

## What runs, and when

Each exercise set is an application of its own, and is tested once every file it asks for exists:

| Set | Tested when | Tests |
|---|---|---|
| `exercise1` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | The program's output, against [`exercise1.txt`](./exercise1.txt) |
| `exercise2` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | The output as built, against [`exercise2.txt`](./exercise2.txt), and built with `-DCONFIG_LOG_DEFAULT_LEVEL=2`, against [`exercise2-warnings.txt`](./exercise2-warnings.txt) |
| `exercise3` | all seven files exist | The output as built, against [`exercise3.txt`](./exercise3.txt), and with `-DEXTRA_CONF_FILE=fahrenheit.conf`, against [`exercise3-fahrenheit.txt`](./exercise3-fahrenheit.txt); and `celsius_to_fahrenheit()` itself, with the ztest application in [`convert/`](./convert) |

A set whose files do not all exist yet is reported as `SKIP`, with the first missing path, so a
file written under the wrong name is visible rather than silently untested.

**How a program is checked.** Each program is built for `native_sim/native/64` and run with
`-no-rt -stop_at=<seconds>`, so it runs in simulated time and stops by itself. Its output is
compared with the expected output line by line, after removing the colors, the
`*** Booting Zephyr OS build ... ***` banner, and the `Stopped at` line that `native_sim` prints at
the end. A program that is still running after 30 seconds of real time is reported as hung.

**How a function is checked.** [`convert/`](./convert) is a ztest application, the test framework
L05 covers. The suite builds it with `-DEXERCISE_DIR` pointing at your `exercise3`, and it compiles
your `src/convert.c` in with its tests. It passes if every test passes.
