# L05 Test Suite

Tests for everything [Appendix C](../../appendix/c_exercises.md) asks you to write, and the Lumo
emulator they run against. The suite is described by [`suite.yaml`](./suite.yaml) and run by
[`ci/suite.py`](../../../../ci/suite.py), which says what every key means.

```bash
python3 ci/suite.py lectures/L05               # Against your code in ../
python3 ci/suite.py --solutions lectures/L05   # Against ../../appendix/solutions
```

`make test` at the repository root runs this suite and every other lecture's.

---

## The emulator
[`lumo_emul/`](./lumo_emul) is a Zephyr module that plays a Lumo chip on `native_sim`'s emulated I2C
bus, for every enabled `"qacademy,lumo"` node. Its register map is written out in
[`lumo_emul.c`](./lumo_emul/src/lumo_emul.c) rather than shared with your driver, so that a wrong
address in your `lumo_reg.h` fails against it. Its Kconfig option, `EMUL_LUMO`, depends on the
devicetree having a `"qacademy,lumo"` node, so it only builds together with your binding.

It gives tests three controls in [`<qacademy/lumo_emul.h>`](./lumo_emul/include/qacademy/lumo_emul.h),
and the generic sensor emulator backend for `SENSOR_CHAN_LIGHT`. With
`CONFIG_EMUL_LUMO_DAYLIGHT=y` it also starts a thread that raises the light level before each whole
second of uptime, which Exercise Set 4 is run with.

---

## What runs, and when

| Set | Tested when | Tests |
|---|---|---|
| `exercise1` | all seven module files exist | [`device/`](./device): the option turns itself on, the part is ready, its API is a sensor API, a wrong `PART_ID` fails init |
| `exercise2` | all seven module files exist | [`device/`](./device), and [`conversion/`](./conversion): counts to lux at a gain of 1, byte order, saturation, the fetch and get contract |
| `exercise3` | all seven module files exist | [`device/`](./device), [`conversion/`](./conversion), and [`gain/`](./gain): the gain in `CONFIG` after boot, and conversions at gains of 4 and 8 |
| `exercise4` | its four files and `exercise3/src/lumo.c` exist | The application, built with the emulator and its sunrise, run for 6.5 simulated seconds: its output must contain the lines of [`exercise4.txt`](./exercise4.txt), in order |
| `exercise5` | its five files and `exercise3/src/lumo.c` exist | Your test application, built with the emulator: its output must contain the lines of [`exercise5.txt`](./exercise5.txt), so your suite `lumo` must pass |

A set whose files do not all exist yet is reported as `SKIP`, with the first missing path, so a
file written under the wrong name is visible rather than silently untested.

**How the driver is tested.** Each directory in the table is a ztest application. The suite
builds it with `-DEXERCISE_DIR` pointing at the set's directory, and the application's
`CMakeLists.txt` adds that directory, your module, and `lumo_emul/` to `ZEPHYR_EXTRA_MODULES`. Its
`prj.conf` does not enable `CONFIG_LUMO`: your Kconfig option must turn itself on from the
devicetree. It passes if every test passes.

**How the applications are tested.** Sets 4 and 5 are your applications, built with
`-DZEPHYR_EXTRA_MODULES` naming `lumo_emul/`, as Appendix C's commands do. Their output is compared
with `match: contains`: every expected line must appear, in order, and the emulator's and ztest's
other lines may appear in between.
