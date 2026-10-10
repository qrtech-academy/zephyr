# Appendix B
## Emulators and Testing
A driver for a part nobody has is untestable, unless something plays the part. This appendix covers
Zephyr's emulators, which do, and the two tools that run tests against them: the ztest framework,
which the tests are written in, and Twister, which builds and runs them. By the end you should be
able to explain every file in [`examples/thermo/module/drivers/sensor/thermo/thermo_emul.c`](../examples/thermo/module/drivers/sensor/thermo/thermo_emul.c)
and [`examples/thermo/tests`](../examples/thermo/tests).

---

## Emulators
### What an emulator is
An **emulator** is a piece of software that stands in for a peripheral chip, at the level of the bus
the chip is on. The driver under test is the real driver, unchanged: it calls
`i2c_reg_read_byte_dt()` as it would on a board. What changes is below it. On `native_sim`, the I2C
controller `i2c0` is `zephyr,i2c-emul-controller`, a driver that, instead of clocking bits onto
wires, hands every transfer to the emulator registered at the transfer's address.

```text
application ── sensor API ──> thermo.c ── I2C API ──> i2c_emul ──> thermo_emul.c
                              (driver)                (i2c0)       (the "chip")
```

That is what makes emulators more useful than mocks: the driver's I2C calls, its register addresses,
its byte order and the order of its transfers all run for real, and only the silicon is software.

The emulated I2C controller needs `CONFIG_EMUL=y`. Without it, `native_sim`'s `i2c0` has no driver,
and a sensor on that bus fails to link: in the exercises' application, with
`undefined reference to '__device_dts_ord_31'`, where node 31 is `i2c@100`.

### Defining one
An emulator is defined per devicetree node, like a device, and usually for the same compatible as the
driver:

```c
#define DT_DRV_COMPAT qacademy_thermo

#define THERMO_EMUL_DEFINE(inst)                                               \
	static struct thermo_emul_data thermo_emul_data_##inst;                \
	EMUL_DT_INST_DEFINE(inst, thermo_emul_init, &thermo_emul_data_##inst, NULL, \
			    &thermo_emul_i2c_api, &thermo_emul_sensor_api)

DT_INST_FOREACH_STATUS_OKAY(THERMO_EMUL_DEFINE)
```

`EMUL_DT_INST_DEFINE()` takes the instance, an init function, the emulator's data and configuration,
the **bus API** it answers transfers with, and an optional **backend API** that tests use to control
it. The result is a `struct emul`, which tests get with `EMUL_DT_GET(node)`, just as they get the
device with `DEVICE_DT_GET(node)`.

When the emulated I2C controller initializes, it looks up an emulator for each of its child nodes
and registers it at the node's address. That is logged at boot:

```text
<inf> emul: Registering 1 emulator(s) for i2c@100
<inf> i2c_emul_ctlr: Register emulator 'lumo@29' at I2C addr 29
```

A node without an emulator is logged as `Cannot find emulator for 'lumo@29'`, and every transfer to
its address fails with `-EIO`: the bus behaves as if nothing answered there. The controller
initializes before the drivers on its bus, so every emulator is in its reset state by the time a
driver's init function reads from it.

### The bus API
For I2C, the bus API is one function:

```c
static const struct i2c_emul_api thermo_emul_i2c_api = {
	.transfer = thermo_emul_transfer,
};
```

It receives the messages of one I2C transfer, exactly as the driver's call produced them. The
common register helpers arrive in two shapes:
* `i2c_reg_write_byte_dt()` is **one write message**: the register address, then the value.
* `i2c_reg_read_byte_dt()` and `i2c_burst_read_dt()` are **two messages**: a write of the register
  address, then a read of one or more bytes, which by convention continue at the following
  registers.

An emulator's transfer function is therefore a small register map: it keeps the registers in RAM,
updates them on writes, and fills in read buffers from them, doing whatever the datasheet says
happens in between, such as clearing a one-shot bit when a conversion finishes. Everything a real
part would get wrong-footed by, such as a write to a read-only register, the emulator decides how
to handle; Thermo's ignores it and logs a warning, as the part would ignore it.

### The backend API
Tests need to set what the part measures. They could call functions of the emulator's own, and
Thermo's emulator does have a header of its own, `thermo_emul.h`, for faults no real part has on
purpose: a wrong `WHO_AM_I`, a conversion that never finishes.

For the measured value itself there is a generic interface, the **sensor emulator backend** in
`<zephyr/drivers/emul_sensor.h>`:

```c
static const struct emul_sensor_driver_api thermo_emul_sensor_api = {
	.set_channel = thermo_emul_set_channel,
	.get_sample_range = thermo_emul_get_sample_range,
};
```

and tests call it through `emul_sensor_backend_set_channel()`, which works the same for every sensor
emulator in Zephyr that implements it. The value is passed as a **Q31** fixed-point fraction with a
shift: the physical value is `value / 2^31 * 2^shift`, in the channel's unit. A shift of 8 covers
plus or minus 256 degrees; a shift of 15, 32768 lux. Converting a value in millionths:

```c
const int8_t shift = 8;
q31_t value = (q31_t)((micro_c * (1LL << (31 - shift))) / 1000000);

emul_sensor_backend_set_channel(
	emul, (struct sensor_chan_spec){SENSOR_CHAN_AMBIENT_TEMP, 0}, &value, shift);
```

The emulator converts the value into what the part would register, rounded as the part would round
it. The test then reads it back through the driver and checks the conversion end to end. The
emulated I2C bus also has a lower-level escape hatch, `mock_api` on `struct i2c_emul`, which lets a
test intercept transfers itself, for faults such as a bus that returns an error.

### What an emulator can and cannot show
An emulator shows whether a driver talks to the part the way the datasheet says: the right
registers, the right byte order, the right conversion, the right reaction to a wrong ID or a timeout.
It shows it deterministically, on every commit, without hardware.

It cannot show what the author of the emulator did not know or did not model: a real part that
needs 30 ms before it answers after power-up, a measurement that changes between reading two
registers, electrical noise, or a datasheet that is wrong. An emulator is a second implementation of
the datasheet, and a driver that passes against it agrees with that implementation. The first run
on a board is still a test.

---

## ztest
### A test application
A ztest test is an ordinary Zephyr application, built for a board, whose `main()` is provided by the
framework. `prj.conf` enables it:

```text
CONFIG_ZTEST=y
```

and the tests are functions in suites:

```c
#include <zephyr/ztest.h>

ZTEST_SUITE(thermo, NULL, NULL, NULL, after, NULL);

ZTEST(thermo, test_zero)
{
	zassert_ok(set_temperature(0));

	struct sensor_value value = read_temperature();

	zassert_equal(value.val1, 0);
	zassert_equal(value.val2, 0);
}
```

On `native_sim`, the program runs every suite, prints a summary, and exits with status 0 if every
test passed and 1 if any failed, which is what Twister, and this course's `make test`, check. Tests
built with ztest also run in simulated time as fast as the host allows: `native_sim` only slows down
to real time by default when the build is not a test.

```text
SUITE PASS - 100.00% [thermo]: pass = 11, fail = 0, skip = 0, total = 11 duration = 0.000 seconds
 - PASS - [thermo.test_channel_get_returns_the_last_fetch] duration = 0.000 seconds
 ...
PROJECT EXECUTION SUCCESSFUL
```

### Suites and hooks
`ZTEST_SUITE(name, predicate, setup, before, after, teardown)` declares a suite and, optionally,
functions that run around it:

| Argument | Runs |
|----------|------|
| `predicate` | Before the suite, to decide whether it runs at all. |
| `setup` | Once, before the suite's first test. Its return value is the suite's *fixture*. |
| `before` | Before every test. |
| `after` | After every test, whether it passed or failed. |
| `teardown` | Once, after the suite's last test. |

`ZTEST(suite, name)` defines a test; `ZTEST_F(suite, name)` defines one that receives the fixture.
Thermo's suite uses `after` to repair whatever fault a test injected, so that a failing test cannot
make the next one fail too.

**Tests run in alphabetical order**, not in the order they appear in the file. A test that only
passes because another ran first is a test that will one day fail for no reason; every test must set
up everything it depends on, and the hooks exist for exactly that.

### Assertions
| Macro | On failure |
|-------|------------|
| `zassert_true(cond, msg, ...)`, `zassert_false`, `zassert_equal(a, b, ...)`, `zassert_ok(expr)`, ... | Fails the test and ends it. |
| `zexpect_true`, `zexpect_equal`, ... | Fails the test, but lets it continue, to report more than the first failure. |
| `zassume_true`, `zassume_equal`, ... | Skips the rest of the test: for a precondition that is not what is being tested. |

Every assertion takes an optional `printf`-style message, which is printed with the file, the line
and the failed condition. A good message says what should have happened, in the terms of the
datasheet or the exercise, not only that two numbers differed:

```text
Assertion failed at .../conversion/src/main.c:71: conversion_test_saturated: (value.val1 not equal to 16383)
0xFFFF counts is 16383.75 lux, not -796.-119184
```

### What to test in a driver
Against an emulator, a driver's tests cover what a datasheet specifies and what a user relies on:
* **Readiness**: the device is ready when the part is genuine, and not when it is not.
* **Conversion at the edges**: zero, the smallest step, negative values if the part has them, and
  the limits of the range, where overflows and sign errors live.
* **Configuration from devicetree**: what a property should put into a register, read back from the
  emulator.
* **The API contract**: unsupported channels, fetch without get and get without fetch.
* **Faults**: a timeout, a wrong ID, a bus error. These are the tests only an emulator makes cheap.

---

## Twister
### Test applications as scenarios
**Twister** is Zephyr's test runner, and the same tool the course's `make examples` uses. It walks a
directory tree, finds every application with a `testcase.yaml` (or, for samples, a `sample.yaml`),
and builds, runs and checks each of its **scenarios** on each selected platform, in parallel.

The Thermo tests' `testcase.yaml`:

```yaml
common:
  platform_allow: native_sim/native/64
  integration_platforms:
    - native_sim/native/64
  harness: ztest
  tags:
    - sensor
tests:
  qacademy.l05.thermo.driver: {}
```

| Key | Meaning |
|-----|---------|
| `tests` | The scenarios, by name. Each builds the application once; keys under a scenario override `common`. |
| `platform_allow` | The platforms the scenario may run on. |
| `integration_platforms` | The platforms it runs on when Twister is run with `--integration`, as Zephyr's CI does for pull requests. |
| `harness: ztest` | How to judge the run: parse ztest's output, and report every test case. A sample uses `harness: console` with regular expressions instead. |
| `extra_configs`, `extra_args` | Kconfig settings and CMake arguments for the scenario, such as `CONFIG_ASSERT=y` or `EXTRA_CONF_FILE=...`. |
| `tags`, `filter` | Selecting scenarios: by tag on the command line, or by an expression over the build's configuration. |

A suite and a scenario are different things. A ztest **suite** is a group of tests inside one
program. A Twister **scenario** is one build of the program: the same tests built twice, once with
`CONFIG_FOO=y` and once without, are two scenarios running the same suites.

### Running it

```bash
west twister -T lectures/L05/examples/thermo/tests -p native_sim/native/64
```

`-T` gives the directory to search, and `-p` the platform. Twister writes everything to
`twister-out/` in the current directory, renaming an older one to `twister-out.1`: one build
directory per scenario, under `twister-out/native_sim_native_64/host_gnu/<path>/<scenario>/`, with
the program's output in `handler.log` and the build's in `build.log`, and reports in
`twister-out/twister.json` and `twister.xml`. `-i` (`--inline-logs`) prints the log of every failure
in the terminal instead.

**Twister is stricter than a plain build.** It builds with compiler warnings as errors
(`CONFIG_COMPILER_WARNINGS_AS_ERRORS=y`) and with devicetree warnings as errors
(`--edtlib-Werror`), which is how a missing `vendor-prefixes.txt` that only warned by hand stops a
Twister run. `--disable-warnings-as-errors` turns both off, and should not be needed.

Extra CMake arguments for every build go after `-x`, for example a module that is not in the
application's `CMakeLists.txt`:

```bash
west twister -T <tests> -p native_sim/native/64 -x=ZEPHYR_EXTRA_MODULES=<path to module>
```

With `--coverage`, Twister builds with gcov instrumentation and collects coverage of the code the
tests ran into an HTML report. L06 shows it.

The same `testcase.yaml` runs on real boards too, with `--device-testing` and a serial port: the
scenarios that pass here against an emulator are what **Hardware-in-the-Loop Testing** runs against
the real part.

---

## Summary
* An emulator plays a peripheral chip at its bus. The driver under test is unchanged; on
  `native_sim`, the emulated I2C controller hands each transfer to the emulator at its address.
* `EMUL_DT_INST_DEFINE()` defines an emulator per devicetree node, with a bus API that answers
  transfers and an optional backend API that tests control it through. `EMUL_DT_GET()` gets it.
* The sensor emulator backend sets a measured value as Q31 with a shift, the same way for every
  sensor emulator. Faults no real part has on purpose go in the emulator's own header.
* An emulator is a second implementation of the datasheet: it tests the driver's agreement with it,
  not with the silicon.
* ztest applications are Zephyr applications; `ZTEST_SUITE()` declares a suite and its hooks,
  `ZTEST()` a test. Tests run in alphabetical order, so none may depend on another.
* `zassert_*` ends a test, `zexpect_*` lets it continue, and `zassume_*` skips it. Say in the
  message what should have happened.
* Twister finds `testcase.yaml` files, builds every scenario, runs it, and judges it by its harness,
  with warnings as errors. A suite is tests in a program; a scenario is a build of a program.

---
