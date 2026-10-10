# L01 - Zephyr, west and the Build

## Agenda
* What Zephyr is: a kernel, drivers, subsystems, and a build system that ties them together.
* `native_sim`: Zephyr as a Linux program, and why the course runs on it.
* The west workspace: the manifest, `west update`, and pinning a Zephyr release.
* Anatomy of an application: `CMakeLists.txt`, `prj.conf` and `src/`.
* Building and running: `west build`, the build directory, and `west build -t run`.
* Kconfig: options, dependencies, where values come from, and how to read what the build chose.
* Application-specific Kconfig options, used from C and from CMake.
* `printk()` and the logging subsystem: modules, levels and compile-time filtering.

---

## Objectives
After this lecture, participants should:
* Be able to set up the course workspace and build and run an application on `native_sim`.
* Understand what the west manifest pins, and where Zephyr lives in the workspace.
* Understand the three files every Zephyr application has, and what each one contributes.
* Understand the order in which a Zephyr build configures itself: board, devicetree, Kconfig, CMake.
* Be able to find the configuration a build ended up with, in `.config` and in menuconfig.
* Be able to add Kconfig options of their own and use them from C and CMake.
* Be able to log at the right level, and to change which levels a build keeps.

---

## Prerequisites
* Good knowledge of C.
* Basic familiarity with a Linux terminal, `make` and CMake.
* Basic RTOS concepts (threads, priorities, blocking) help, but are covered again in L03 and L04.

---

## Instructions

### Preparation
* Install the tools and set up the workspace as described in [Appendix A](./appendix/a_setup.md).
  Do this **before** the lecture: fetching Zephyr takes a few minutes, and a slow network more.
* Read [Appendix B](./appendix/b_zephyr_and_the_build.md).

### During the Lecture
* Participate in the lecture (live coding session). The examples are in
  [`examples/`](./examples):
    * [`hello`](./examples/hello/README.md): printing, logging, sleeping and the uptime clock.
    * [`kconfig`](./examples/kconfig/README.md): an application with Kconfig options of its own.

### After the Lecture
* Complete the exercises in [Appendix C](./appendix/c_exercises.md).
* Write them in [`exercises/`](./exercises/README.md) and check them with `make test` from the
  repository root.
* Exercise solutions are published in `appendix/solutions` a few days after the lecture.

---

## Evaluation
Participants should be able to explain:
* What does west do, and what does the manifest `west.yml` pin?
* What are the roles of `CMakeLists.txt`, `prj.conf` and devicetree in a Zephyr build?
* Where does a Kconfig option's final value come from, and where can you read it after a build?
* Why does the build abort when `prj.conf` sets an option whose dependencies are not met?
* Why is a misspelled `CONFIG_` macro in C more dangerous than a misspelled option in `prj.conf`?
* What is the difference between `#ifdef CONFIG_X` and `IS_ENABLED(CONFIG_X)`?
* Why does `k_msleep(1000)` on `native_sim` wake up after 1010 ms?
* What does a log module's level control, and why is a filtered-out message not in the binary?

---

## Next Lecture
Devicetree:
* How a board and its peripherals are described: nodes, properties, labels and aliases.
* Bindings, which say what a node's properties mean.
* Overlays, which change a board's devicetree for one application.
* Getting devices and properties from C at compile time.

Participants will blink an LED and read a button through Zephyr's emulated GPIO controller.

---
