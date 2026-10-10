# Appendix C

## Exercises
These exercises reinforce the concepts from [Appendix A](./a_device_drivers.md) and
[Appendix B](./b_emulators_and_testing.md).

> **How to check your work.** Write each exercise set in its own directory under
> [`lectures/L05/exercises`](../exercises/README.md), with the file names given there, and run
> `make test` from the repository root. A set's tests switch on as soon as its files exist, and a
> set you have not started is reported as `SKIP`. Sets 1 to 3 are one driver growing: start each
> by copying the previous set's directory. The tests of Sets 1 to 3 are ztest applications in
> [`exercises/test`](../exercises/test/README.md) that build your module against the course's
> emulator; Set 4 is checked by its output, and Set 5 by running your own tests.

Remember `source env.sh` in every new terminal.

---

## The part: Lumo
Lumo is a made-up I2C ambient light sensor. It measures continuously: its DATA registers always hold
the latest measurement, and there is nothing to start. Its datasheet, in full:

| Register | Name | Access | Contents |
|----------|------|--------|----------|
| `0x00` | `CONFIG` | R/W | Bits 1:0, `GAIN`: `0` is a gain of 1, `1` of 2, `2` of 4, `3` of 8. Bits 7:2 are reserved, read as 0 and ignore writes. Reset value `0x00`. |
| `0x01` | `DATA_LSB` | R | The measurement, low byte. |
| `0x02` | `DATA_MSB` | R | The measurement, high byte. |
| `0x0A` | `PART_ID` | R | Always `0x4C` on a genuine part. |

* The measurement is an **unsigned 16-bit count, little-endian**: `DATA_LSB` then `DATA_MSB`.
* At a gain of 1, **one count is 0.25 lux**. At a gain of G, one count is 0.25 / G lux.
* A count of `0xFFFF` means the part is **saturated**: the light is at least that bright.
* A read of several bytes continues at the following registers, so both DATA bytes can be read in
  one transfer. Writes to read-only registers are ignored.

The course provides the part itself: an emulator, in
[`exercises/test/lumo_emul`](../exercises/test/lumo_emul), which is a Zephyr module of its own. It
plays a Lumo chip for every enabled `"qacademy,lumo"` node on `native_sim`'s I2C bus. The tests add it
to their builds; to build with it by hand, add it on the command line:

```bash
-DZEPHYR_EXTRA_MODULES=$PWD/lectures/L05/exercises/test/lumo_emul
```

Tests control it through [`<qacademy/lumo_emul.h>`](../exercises/test/lumo_emul/include/qacademy/lumo_emul.h)
(`lumo_emul_set_part_id()`, `lumo_emul_set_counts()`, `lumo_emul_get_config()`) and through the
generic sensor emulator backend, with `SENSOR_CHAN_LIGHT` in lux.

---

# Exercise Set 1 – A module and a device
Create a directory named `exercise1` to work in for this exercise set. The directory is your module.

## Exercise 1.1 – The module and its Kconfig option
In this exercise you will create a module that the build can include.

### Tasks
Create these files in `exercise1`:
* `zephyr/module.yml`: a module named `qacademy-lumo`, whose CMake and Kconfig are in the module's
  top directory, and whose `dts_root` is the top directory too.
* `Kconfig`: an option `LUMO`, a `bool` with the prompt `Lumo I2C ambient light sensor`, which:
  * is `y` by default,
  * depends on `SENSOR`, and on the devicetree having an enabled `"qacademy,lumo"` node,
  * selects `I2C`.
* `CMakeLists.txt`: when `CONFIG_LUMO` is enabled, a Zephyr library holding `src/lumo.c`.

---

## Exercise 1.2 – The binding
### Tasks
* Create `dts/bindings/qacademy,lumo.yaml`: a binding for the compatible `"qacademy,lumo"`, which
  includes `sensor-device.yaml` and `i2c-device.yaml`, and has no properties of its own yet.
* Create `dts/bindings/vendor-prefixes.txt`, registering the vendor prefix `qacademy`.

---

## Exercise 1.3 – The driver's init
### Tasks
* Create `src/lumo_reg.h` with macros for the register addresses, the `GAIN` mask, and the value of
  `PART_ID`, from the datasheet above.
* Create `src/lumo.c`, the driver for the compatible `"qacademy,lumo"`:
  * A config struct holding the part's `struct i2c_dt_spec`.
  * A sensor API table, defined with `DEVICE_API(sensor, ...)`, whose `sample_fetch` and
    `channel_get` functions only return `-ENOTSUP` for now.
  * An init function that:
    * returns `-ENODEV` if the I2C bus is not ready,
    * reads `PART_ID`, and returns the error if the read fails,
    * returns `-ENODEV` if `PART_ID` is not `0x4C`,
    * and otherwise returns 0.
  * One device per enabled instance, defined with `SENSOR_DEVICE_DT_INST_DEFINE()` at `POST_KERNEL`
    and `CONFIG_SENSOR_INIT_PRIORITY`. The data pointer may be `NULL` for now.

The tests in [`test/device`](../exercises/test/device) put one part at address `0x29`, and a
second at `0x2a` whose initialization is deferred, so that a test can give it the wrong `PART_ID`
first. To build and run them by hand:

```bash
west build -p -b native_sim/native/64 -d build/device \
    lectures/L05/exercises/test/device -- \
    -DEXERCISE_DIR=$PWD/lectures/L05/exercises/exercise1
./build/device/zephyr/zephyr.exe
```

The four tests must pass: the option turns itself on, the part is ready, its device has a sensor
API, and the wrong `PART_ID` makes `device_init()` return `-ENODEV`.

### Reflection
* Remove `vendor-prefixes.txt` and build the tests again by hand. What does the build print? Then
  build once more with `-DEXTRA_GEN_EDT_ARGS=--edtlib-Werror` added, as Twister does. What happens?
* Run `west build -d build/device -t initlevels`. Where do your two `lumo_init` calls come in the
  list, and what comes just before them? Why must it be in that order?
* The deferred part is in the `initlevels` list too. Why is it still not ready after boot?
* Find `DT_HAS_QACADEMY_LUMO_ENABLED` in the build directory. Which file defines it, and why could it
  not exist before you wrote the binding?

---

# Exercise Set 2 – Reading light
Copy `exercise1` to a directory named `exercise2` to work in for this exercise set.

## Exercise 2.1 – Fetch and get
In this exercise you will implement the sensor API, at the part's reset gain of 1.

### Tasks
In `src/lumo.c`:
* Add a data struct holding the count of the last fetch, and pass it to the device.
* `sample_fetch` shall:
  * return `-ENOTSUP` for any channel other than `SENSOR_CHAN_ALL` and `SENSOR_CHAN_LIGHT`,
  * read both DATA bytes **in one transfer**, and return the error if the read fails,
  * store the count, and return 0.
* `channel_get` shall:
  * return `-ENOTSUP` for any channel other than `SENSOR_CHAN_LIGHT`,
  * convert the stored count to lux, at 0.25 lux per count, into the `struct sensor_value`.
* Do no I/O in `channel_get`.

The tests in [`test/conversion`](../exercises/test/conversion) check, among others, that:

| Count | Lux | `val1` | `val2` |
|-------|-----|--------|--------|
| `0` | 0 | `0` | `0` |
| `1` | 0.25 | `0` | `250000` |
| `0x0102` | 64.5 | `64` | `500000` |
| `0xFFFF` | 16383.75 | `16383` | `750000` |

The tests of Set 1 must still pass.

**Hint:** `sys_get_le16()`, from `<zephyr/sys/byteorder.h>`, turns two little-endian bytes into a
`uint16_t`. `sensor_value_from_micro()` fills in a `struct sensor_value` from millionths.

### Reflection
* Replace the single transfer with two `i2c_reg_read_byte_dt()` calls, one per DATA byte. Do the
  tests still pass? What could go wrong on a real part that the emulator cannot show?
* Compute the lux in `int` instead of `int64_t`, by removing the cast. Which test fails? Above how
  many counts does the multiplication overflow, and how bright is that?
* `test_channel_get_returns_the_last_fetch` changes the part's count between a fetch and a get. Why
  is the old value the right answer?

---

# Exercise Set 3 – The gain
Copy `exercise2` to a directory named `exercise3` to work in for this exercise set.

## Exercise 3.1 – A devicetree property
In this exercise you will make the gain a property of the devicetree node, written to the part at
init.

### Tasks
* In the binding, add a property `gain`: an `int`, allowed to be only 1, 2, 4 or 8, with a default
  of 1.
* In `src/lumo.c`:
  * Add the gain and its `GAIN` bits to the config struct. Take the bits from the property's index in
    the binding's list of allowed values, at compile time.
  * In the init function, after checking `PART_ID`, write the `GAIN` bits to `CONFIG`, without
    changing the register's other bits, and return the error if that fails.
  * In `channel_get`, divide by the gain: at a gain of G, one count is 0.25 / G lux.

The tests in [`test/gain`](../exercises/test/gain) put three parts on the bus, at gains of 4, 8 and
the default, and check what each one's `CONFIG` register holds after boot and what its counts
convert to: one count at a gain of 4 is 0.0625 lux, and `0xFFFF` at a gain of 8 is 2047.96875 lux.
The tests of Sets 1 and 2 must still pass.

**Hint:** `DT_INST_ENUM_IDX(inst, prop)` is the index of a property's value in its binding's `enum`
list. `i2c_reg_update_byte_dt()` changes only the bits of a register that a mask selects.

### Reflection
* Add `gain = <3>;` to a part in an overlay and build the gain tests. What happens, and at which
  step of the build?
* Why take the `GAIN` bits from `DT_INST_ENUM_IDX()`, rather than computing a base-2 logarithm of the
  gain at run time?
* At a gain of 8, what is the smallest step between two readings, and the brightest light the part
  can measure? What are they at a gain of 1?

---

# Exercise Set 4 – An application
Create an application named `exercise4` to work in for this exercise set.

## Exercise 4.1 – Reading once a second
### Tasks
* `CMakeLists.txt`: add the module in `../exercise3` to the build, before finding Zephyr. Do **not**
  add the emulator.
* `prj.conf`: enable the sensor subsystem, emulators, and logging.
* `app.overlay`: a Lumo part labelled `light`, at address `0x29` on `&i2c0`, at the default gain.
* `src/main.c`, with a log module named `light_app` at level `INF`:
  * If the part is not ready, log `<name> is not ready` as an error, with the device's name, and
    return.
  * Log `Reading <name>`.
  * Then, forever: fetch and get a light reading, log it as `<lux> lux` with two decimals, or
    `Reading failed: <error>` as an error, and sleep for one second.

Build it with the emulator, and with the emulator's sunrise, which changes the light half a second
before each whole second:

```bash
west build -p -b native_sim/native/64 lectures/L05/exercises/exercise4 -- \
    -DZEPHYR_EXTRA_MODULES=$PWD/lectures/L05/exercises/test/lumo_emul \
    -DCONFIG_EMUL_LUMO_DAYLIGHT=y
./build/zephyr/zephyr.exe -stop_at=6.5
```

The output must contain, in this order, with the emulator's own log lines in between:

```text
[00:00:00.000,000] <inf> light_app: Reading lumo@29
[00:00:00.000,000] <inf> light_app: 0.00 lux
[00:00:01.010,000] <inf> light_app: 0.25 lux
[00:00:02.020,000] <inf> light_app: 42.00 lux
[00:00:03.030,000] <inf> light_app: 320.00 lux
[00:00:04.040,000] <inf> light_app: 2750.00 lux
[00:00:05.050,000] <inf> light_app: 12000.00 lux
[00:00:06.060,000] <inf> light_app: 16383.75 lux
```

### Reflection
* Build the application without `-DZEPHYR_EXTRA_MODULES=...`. Does it build? What does it print,
  and why?
* Build it with the emulator, but with `-DCONFIG_EMUL=n` added. What happens, and which devicetree
  node is the error about?
* The emulator's last light level is 30000 lux, but the reading is 16383.75. Why? Set `gain = <8>`
  in `app.overlay` and run it again: what do the readings become, and what does that say about
  choosing a gain?

---

# Exercise Set 5 – Tests of your own
Create a directory named `exercise5` to work in for this exercise set: a ztest application that
tests the driver of Exercise Set 3.

## Exercise 5.1 – A test suite and a Twister scenario
### Tasks
* `CMakeLists.txt`: add the module in `../exercise3` to the build, and `src/main.c` to the
  application. As in Set 4, do not add the emulator.
* `prj.conf`: enable ztest, the sensor subsystem and emulators.
* `app.overlay`: two Lumo parts, one at the default gain and one at a gain of 2.
* `src/main.c`: a suite named `lumo`, whose tests check at least:
  * that both parts are ready,
  * counts of 0, 1 and `0xFFFF` at the default gain,
  * a count at a gain of 2,
  * that the gain of 2 was written to the part's `CONFIG` register,
  * and that an unsupported channel returns `-ENOTSUP` from both fetch and get.
* `testcase.yaml`: one scenario, named `qacademy.l05.exercise5.lumo`, allowed only on
  `native_sim/native/64`, judged by the ztest harness.

Build and run it with the emulator:

```bash
west build -p -b native_sim/native/64 lectures/L05/exercises/exercise5 -- \
    -DZEPHYR_EXTRA_MODULES=$PWD/lectures/L05/exercises/test/lumo_emul
./build/zephyr/zephyr.exe
```

The suite must pass: the output must contain `SUITE PASS - 100.00% [lumo]` and
`PROJECT EXECUTION SUCCESSFUL`.

### Reflection
* Run the scenario with Twister:

  ```bash
  west twister -T lectures/L05/exercises/exercise5 -p native_sim/native/64 \
      -x=ZEPHYR_EXTRA_MODULES=$PWD/lectures/L05/exercises/test/lumo_emul
  ```

  Where does Twister put the output of your test program? What does it report besides pass and
  fail?
* Break your Set 3 driver on purpose: make `channel_get` ignore the gain. Which of your tests fail?
  If none do, which test is missing?
* Your tests and the course's tests both pass. What would you still check on the first board with a
  real Lumo part?

---
