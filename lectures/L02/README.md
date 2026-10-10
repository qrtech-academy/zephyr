# L02 - Devicetree

## Agenda
* Why Zephyr describes hardware in data rather than in code.
* Devicetree syntax: nodes, properties, unit addresses, labels, phandles, aliases and `chosen`.
* The board's devicetree: `native_sim` and its emulated peripherals.
* Overlays: changing the board's devicetree for one application, and which overlays a build uses.
* `/zephyr,user`: values of the application's own, without a binding.
* Bindings: what a node's properties mean, which are required, and their types.
* From devicetree to C: node identifiers, `DT_ALIAS()`, `DT_NODELABEL()`, `DT_PROP()`, and
  `DT_FOREACH_STATUS_OKAY()`.
* Getting a device: `DEVICE_DT_GET()`, `GPIO_DT_SPEC_GET()` and `device_is_ready()`.
* GPIO: outputs, inputs and edge interrupts, through the emulated GPIO controller.
* Reading devicetree errors: `__device_dts_ord_` and friends.

---

## Objectives
After this lecture, participants should:
* Understand how devicetree separates *what hardware there is* from *the code that uses it*.
* Be able to read a board's devicetree, and the merged result of a build in `zephyr.dts`.
* Be able to add and change nodes and properties with an overlay.
* Be able to write a binding for a node of their own.
* Be able to get devices and properties from the devicetree in C, entirely at compile time.
* Be able to drive and read GPIO pins, and react to edges with a callback.
* Be able to trace a devicetree-related build error back to the node that caused it.

---

## Prerequisites
* L01: the workspace, `west build`, Kconfig.
* Basic familiarity with GPIO: outputs, inputs, pull-ups, active-low signals and edge interrupts.

---

## Instructions

### Preparation
* Read [Appendix A](./appendix/a_devicetree.md).
* Open `deps/zephyr/boards/native/native_sim/native_sim.dts`, the devicetree of the board the course
  builds for, and find the LED, the GPIO controller and the I2C bus.

### During the Lecture
* Participate in the lecture (live coding session). The examples are in
  [`examples/`](./examples):
    * [`blinky`](./examples/blinky/README.md): an LED from the board's devicetree, read back through
      the emulated GPIO controller, and the two errors a missing device produces.
    * [`button`](./examples/button/README.md): a button added with an overlay, an edge interrupt
      callback, and a thread that presses the emulated button.
    * [`bindings`](./examples/bindings/README.md): a binding of the application's own, and a table
      of nodes built at compile time.

### After the Lecture
* Complete the exercises in [Appendix B](./appendix/b_exercises.md).
* Write them in [`exercises/`](./exercises/README.md) and check them with `make test` from the
  repository root.
* Exercise solutions are published in `appendix/solutions` a few days after the lecture.
* For further reading, the Zephyr documentation's
  [Introduction to devicetree](https://docs.zephyrproject.org/4.4.0/build/dts/intro.html),
  [Devicetree bindings](https://docs.zephyrproject.org/4.4.0/build/dts/bindings.html),
  [Devicetree HOWTOs](https://docs.zephyrproject.org/4.4.0/build/dts/howtos.html) and
  [Devicetree access from C/C++](https://docs.zephyrproject.org/4.4.0/build/dts/api-usage.html)
  are the reference for everything in this lecture.

---

## Evaluation
Participants should be able to explain:
* What is the difference between a node's name, its label, and an alias?
* What does a binding add to a devicetree node, and what happens to a node whose binding is missing?
* Why must a `compatible` string's vendor prefix be known to the build?
* How does an overlay change a board's devicetree, and where can you see the merged result?
* Why is `DT_PROP(node, prop)` resolved at compile time, and what does that make impossible?
* What does `device_is_ready()` check, and why must a driver's user call it?
* What does `GPIO_ACTIVE_LOW` in the devicetree change in the C code that uses the pin?
* What is the difference between `‘__device_dts_ord_32’ undeclared` and
  `undefined reference to '__device_dts_ord_32'`, and what does the number tell you?

---

## Next Lecture
Kernel I, threads and time:
* Threads, priorities, and cooperative and preemptive scheduling.
* Timers, and which context their expiry functions run in.
* Work queues, and deferring work out of interrupt context.
* What `native_sim`'s simulated time means for code that spins, and for stack sizes.

Participants will trace, line by line, which thread runs when, and why.

---
