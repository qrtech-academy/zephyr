# L06 Test Suite

Tests for everything [Appendix C](../../appendix/c_exercises.md) asks you to write. The suite is
described by [`suite.yaml`](./suite.yaml) and run by [`ci/suite.py`](../../../../ci/suite.py),
which says what every key means.

```bash
python3 ci/suite.py lectures/L06               # Against your code in ../
python3 ci/suite.py --solutions lectures/L06   # Against ../../appendix/solutions
```

`make test` at the repository root runs this suite and every other lecture's.

---

## What runs, and when

Each exercise set is an application of its own, and is tested once every file it asks for exists:

| Set | Tested when | Tests |
|---|---|---|
| `exercise1` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | The commands in [`exercise1.in`](./exercise1.in), typed into the shell; the output must contain [`exercise1.txt`](./exercise1.txt) |
| `exercise2` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | Five runs of one build, sharing one simulated flash: set, get, a run with `-flash_in_ram`, reset, and a run after the reset, each against its `exercise2-*.in` and `exercise2-*.txt` |
| `exercise3` | `CMakeLists.txt`, `prj.conf`, `src/main.c` and `src/fatal.c` exist | As built, exit status 0 and [`exercise3.txt`](./exercise3.txt); with `-DCONFIG_ASSERT=y`, exit status 1 and [`exercise3-assert.txt`](./exercise3-assert.txt) |
| `exercise4` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | As built, and with `-DCONFIG_ASAN=y -DCONFIG_UBSAN=y`, the output must be exactly [`exercise4.txt`](./exercise4.txt) |
| `exercise5` | `CMakeLists.txt`, `prj.conf` and `src/main.c` exist | The client's output must contain [`exercise5.txt`](./exercise5.txt) |

A set whose files do not all exist yet is reported as `SKIP`, with the first missing path, so a
file written under the wrong name is visible rather than silently untested.

**How a program is checked.** Each program is built for `native_sim/native/64` and run with
`-no-rt -stop_at=<seconds>`, so it runs in simulated time and stops by itself. Its standard input is
the `.in` file, or empty. Its output, standard output and standard error together, is compared
after removing the colors, the `*** Booting Zephyr OS build ... ***` banner, and the `Stopped at`
line that `native_sim` prints at the end:
* **Sets 1, 2, 3 and 5 are checked with `contains`**: every line of the expected file must be part of
  a line of the output, in the same order, with anything in between. The shell's prompts and echoed
  commands, the kernel's thread addresses, and the network's entropy warning may all be there.
* **Set 4 is checked exactly**, line by line. A correct program prints the same with the sanitizers
  as without, so a remaining UndefinedBehaviorSanitizer `runtime error` line fails the test, and so
  does AddressSanitizer's exit status of 1.

**How the flash is kept.** Every build's runs start in a new, empty directory, which is the working
directory of all of that build's runs. `native_sim` writes `flash.bin` there, so Set 2's second run
finds what its first run stored, and no run finds anything a previous `make test` left behind.

**Set 5 uses the network.** Its server binds UDP port 4300 on the host. If another program has that
port, such as a copy of your program still running in another terminal, the server prints
`Server: cannot bind port 4300: 98` (`EADDRINUSE`), and whatever replies the client gets come from
the other program. Stop it, and run the tests again.
