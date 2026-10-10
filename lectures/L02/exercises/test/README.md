# L02 Test Suite

Tests for everything [Appendix B](../../appendix/b_exercises.md) asks you to write. The suite is
described by [`suite.yaml`](./suite.yaml) and run by [`ci/suite.py`](../../../../ci/suite.py),
which says what every key means.

```bash
python3 ci/suite.py lectures/L02               # Against your code in ../
python3 ci/suite.py --solutions lectures/L02   # Against ../../appendix/solutions
```

`make test` at the repository root runs this suite and every other lecture's.

---

## What runs, and when

Each exercise set is an application of its own, and is tested once every file it asks for exists:

| Set | Tested when | Tests |
|---|---|---|
| `exercise1` | `CMakeLists.txt`, `prj.conf`, `app.overlay` and `src/main.c` exist | The output as built, against [`exercise1.txt`](./exercise1.txt), and with [`exercise1-extra.overlay`](./exercise1-extra.overlay), against [`exercise1-extra.txt`](./exercise1-extra.txt) |
| `exercise2` | `CMakeLists.txt`, `prj.conf`, `app.overlay` and `src/main.c` exist | The output as built, against [`exercise2.txt`](./exercise2.txt), and with [`exercise2-extra.overlay`](./exercise2-extra.overlay), against [`exercise2-extra.txt`](./exercise2-extra.txt) |
| `exercise3` | all six files exist, the binding and `vendor-prefixes.txt` included | The output as built, against [`exercise3.txt`](./exercise3.txt), and with [`exercise3-extra.overlay`](./exercise3-extra.overlay), against [`exercise3-extra.txt`](./exercise3-extra.txt) |

A set whose files do not all exist yet is reported as `SKIP`, with the first missing path, so a
file written under the wrong name is visible rather than silently untested.

**How a program is checked.** Each program is built for `native_sim/native/64` and run with
`-no-rt -stop_at=<seconds>`, so it runs in simulated time and stops by itself. Its output is
compared with the expected output line by line, after removing the colors, the
`*** Booting Zephyr OS build ... ***` banner, and the `Stopped at` line that `native_sim` prints at
the end. A program that is still running after 30 seconds of real time is reported as hung.

**Why every set is built twice.** The second build merges an overlay from this directory with
`-DEXTRA_DTC_OVERLAY_FILE`, on top of your `app.overlay`. It changes the hardware, not the task:

| Overlay | Changes |
|---|---|
| [`exercise1-extra.overlay`](./exercise1-extra.overlay) | Points `led0` at a new, active-low LED on pin 5, and blinks it twice, for 50 and 150 ms. |
| [`exercise2-extra.overlay`](./exercise2-extra.overlay) | Moves `button0` to pin 7, active high with a pull-down, and `led1` to pin 3, active low. |
| [`exercise3-extra.overlay`](./exercise3-extra.overlay) | Deletes `warm`, enables `fan`, and gives `overheat` a threshold of 60 and a hysteresis of 10. |

A program that takes pins, polarities, timings and nodes from the devicetree passes both builds
without a change. One that has any of them written into its C code passes the first and fails the
second, and the difference in the output shows which. The overlays refer to your nodes by their
labels, `button0`, `led1`, `overheat`, `warm` and `fan`, which is why the exercises ask for exactly
those.
