# Thermo
A driver for Thermo, a made-up I2C temperature sensor, written the way drivers are written in
Zephyr's own tree: a devicetree binding, a Kconfig option that turns itself on when the devicetree
has the part, one device per devicetree node, and the generic sensor API. The part does not exist,
so the example also has an emulator that plays the chip on `native_sim`'s emulated I2C bus, an
application that reads it, and ztest tests that drive the emulator into states a real part only
reaches when it is broken.

```text
thermo/
    module/     A Zephyr module: the binding, the driver, and the emulator.
    app/        An application that reads the sensor once a second.
    tests/      A ztest suite for the driver, run against the emulator.
```

The module is what a team would keep in a repository of its own. The application and the tests both
use it the same way: one line in their `CMakeLists.txt`, before `find_package(Zephyr)`.

```cmake
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../module)
```

---

## The application

```bash
west build -p -b native_sim/native/64 lectures/L05/examples/thermo/app
west build -t run
```

The sensor is carried into a freezer, one reading per second:

```text
[00:00:00.000,000] <inf> thermo_app: Reading thermo@48
[00:00:00.000,000] <inf> thermo_app: Temperature: 21.5000 C
[00:00:01.010,000] <inf> thermo_app: Temperature: 21.4375 C
[00:00:02.020,000] <inf> thermo_app: Temperature: 12.2500 C
[00:00:03.030,000] <inf> thermo_app: Temperature: 3.5000 C
[00:00:04.040,000] <inf> thermo_app: Temperature: -0.5000 C
[00:00:05.050,000] <inf> thermo_app: Temperature: -12.0000 C
[00:00:06.060,000] <inf> thermo_app: Temperature: -18.0625 C
```

[`app/src/main.c`](./app/src/main.c) only uses the sensor API, and would run unchanged on a board
with a real part. [`app/src/world.c`](./app/src/world.c) is the freezer: it sets the temperature
the emulator will measure, and is only built when `CONFIG_EMUL=y`.

---

## The tests

```bash
west build -p -b native_sim/native/64 lectures/L05/examples/thermo/tests
west build -t run
```

or through Twister, which builds, runs and reports every test application under a directory:

```bash
west twister -T lectures/L05/examples/thermo/tests -p native_sim/native/64
```

Twister writes its builds and reports to `twister-out/` in the current directory. The eleven tests
cover readings above, at and below zero, rounding to the part's 1/16-degree steps, the limits of its
range, a conversion that never finishes, and a part that answers with the wrong `WHO_AM_I`.

---

## What to look at
* **[`thermo_reg.h`](./module/drivers/sensor/thermo/thermo_reg.h) is the datasheet.** Driver and
  emulator both include it, so they cannot disagree about the register map.
* **`CONFIG_THERMO` is not set anywhere.** It defaults to `y` because it depends on
  `DT_HAS_QACADEMY_THERMO_ENABLED`, which the build generates from the devicetree: add the node, get
  the driver. Look for both in `build/zephyr/.config`.
* **Remove the `ZEPHYR_EXTRA_MODULES` line** from the application's `CMakeLists.txt`. The
  devicetree still builds, since a node without a binding is legal, but linking fails with
  `undefined reference to '__device_dts_ord_32'`: nothing defined a device for the node. The number
  is the node's position in the dependency ordering at the top of
  `build/zephyr/include/generated/zephyr/devicetree_generated.h`, where `32` is
  `/i2c@100/thermo@48` in this build.
* **Remove `CONFIG_SENSOR=y`** from the application's `prj.conf`. The link fails the same way, with no
  Kconfig warning: `THERMO` depends on `SENSOR`, and a default that does not take is not an
  assignment, so there is nothing to warn about.
* **[`vendor-prefixes.txt`](./module/dts/bindings/vendor-prefixes.txt)** registers `qacademy`.
  Without it, a plain build prints `compatible 'qacademy,thermo' has unknown vendor prefix
  'qacademy'` and carries on, but Twister builds with `--edtlib-Werror` and stops.
* **Init order.** `west build -t initlevels` lists `i2c_emul_init` before `thermo_init` in
  `POST_KERNEL`: I2C controllers initialize at `CONFIG_I2C_INIT_PRIORITY` (50), sensors at
  `CONFIG_SENSOR_INIT_PRIORITY` (90). The emulated controller registers its emulators while it
  initializes, so the emulated part is in its reset state before the driver reads `WHO_AM_I`.
* **A failed init is final.** `test_wrong_who_am_i_fails_init` defers one device's init with
  `zephyr,deferred-init`, breaks its emulator, and calls `device_init()`: it returns `-ENODEV`,
  `device_is_ready()` returns false, and a second `device_init()` returns `-EALREADY` even after
  the emulator is repaired.
* **Fetch and get are split on purpose.** `sensor_sample_fetch()` does the I/O and stores a reading;
  `sensor_channel_get()` only converts it. `test_channel_get_returns_the_last_fetch` shows that a
  get without a fetch returns the old value.
* **Negative values carry the sign in both parts.** -0.5 degrees is `val1 = 0, val2 = -500000`,
  which is why `main.c` converts to one integer before printing.
* **Timeouts run in simulated time.** `test_conversion_timeout` gives up after 60 ms (50 ms of
  `conversion-timeout-ms`, rounded up by 10 ms ticks) and takes no noticeable time to run.
* **The emulator has two interfaces.** Tests set the temperature through
  `emul_sensor_backend_set_channel()`, the backend every Zephyr sensor emulator can implement, and
  break the part through [`thermo_emul.h`](./module/include/qacademy/thermo_emul.h), which only
  this emulator has.
* **ztest runs the tests in alphabetical order**, not in the order they appear in the file, so no
  test may depend on another having run first; the suite's `after` function undoes any fault a test
  injected.

---
