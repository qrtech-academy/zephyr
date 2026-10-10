# L05 - Writing a Driver

## Agenda
* The device driver model: `struct device`, the API struct, and per-instance config and data.
* Defining devices from the devicetree: `DT_DRV_COMPAT`, `DEVICE_DT_INST_DEFINE()` and
  `DT_INST_FOREACH_STATUS_OKAY()`.
* Initialization: init levels, priorities, what `device_is_ready()` reports after a failure, and
  deferring a device's initialization with `zephyr,deferred-init` and `device_init()`.
* A binding and a Kconfig option for a new part, and a Kconfig option that turns itself on when the
  devicetree has the part.
* Implementing a subsystem API: the sensor API's `sample_fetch` and `channel_get`.
* Zephyr modules: keeping drivers out of tree, with `zephyr/module.yml`.
* Emulators: a chip in software, on `native_sim`'s emulated I2C bus, and the sensor emulator
  backend.
* ztest: suites, tests, assertions, and hooks that run around every test.
* Twister: `testcase.yaml`, scenarios, and running a test suite in one command.

---

## Objectives
After this lecture, participants should:
* Understand how Zephyr turns a devicetree node into a `struct device` at compile time.
* Be able to write a driver that serves every instance of its compatible in the devicetree.
* Understand the order devices are initialized in, and how to find it for a build.
* Be able to implement a subsystem API, so that applications use the part without knowing it.
* Be able to package drivers, bindings and Kconfig as a Zephyr module.
* Be able to write an emulator that lets a driver run, and fail, without hardware.
* Be able to test a driver with ztest, and run the tests with Twister.

---

## Prerequisites
* L02: devicetree and bindings.
* L03 and L04: the kernel, for timeouts.
* Basic familiarity with I2C: addresses, registers, reads and writes.

---

## Instructions

### Preparation
* Read [Appendix A](./appendix/a_device_drivers.md) and
  [Appendix B](./appendix/b_emulators_and_testing.md).
* For more depth, the Zephyr documentation's
  [Device Driver Model](https://docs.zephyrproject.org/4.4.0/kernel/drivers/index.html),
  [Sensors](https://docs.zephyrproject.org/4.4.0/hardware/peripherals/sensor/index.html) and
  [Test Framework](https://docs.zephyrproject.org/4.4.0/develop/test/ztest.html).

### During the Lecture
* Participate in the lecture (live coding session). The example is
  [`examples/thermo`](./examples/thermo/README.md), in three parts:
    * `module/`: the binding, the driver, and an emulator for the part, as a Zephyr module.
    * `app/`: an application that reads the sensor through the generic sensor API.
    * `tests/`: a ztest suite for the driver, run against the emulator with Twister.

### After the Lecture
* Complete the exercises in [Appendix C](./appendix/c_exercises.md): a driver, an application and
  tests for Lumo, a made-up I2C light sensor.
* Write them in [`exercises/`](./exercises/README.md) and check them with `make test` from the
  repository root.
* Exercise solutions are published in `appendix/solutions` a few days after the lecture.
* For more depth, the Zephyr documentation's
  [Emulators](https://docs.zephyrproject.org/4.4.0/hardware/emulator/index.html),
  [Twister](https://docs.zephyrproject.org/4.4.0/develop/test/twister.html) and
  [Modules](https://docs.zephyrproject.org/4.4.0/develop/modules.html).

---

## Evaluation
Participants should be able to explain:
* What is the difference between a device's `config` and its `data`, and where does each one live?
* How does `DT_DRV_COMPAT` connect a driver to the devicetree nodes it serves?
* What decides the order in which devices are initialized, and how can you see it for a build?
* What does `device_is_ready()` return for a device whose init function failed?
* Why does the driver's Kconfig option depend on `DT_HAS_QACADEMY_THERMO_ENABLED`?
* Why does a sensor driver split reading into `sample_fetch` and `channel_get`?
* What does an emulator make testable that a real part does not?
* What is the difference between a ztest suite and a Twister scenario?
* What goes wrong without the module's `vendor-prefixes.txt`, and why only under Twister?

---

## Next Lecture
Subsystems and debugging:
* The shell, with commands of your own.
* Settings that survive a restart, on the simulated flash.
* Networking through the host's sockets.
* Debugging on the host: GDB, sanitizers, fatal errors and coverage.

Participants will make the program talk back, remember things, and crash usefully.

---
