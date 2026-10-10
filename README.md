# Zephyr RTOS
Repository for the course **Zephyr RTOS**.

This course consists of six lectures and is intended for embedded C developers who want to learn
how to build firmware with Zephyr. Every program in it builds and runs on Linux, on Zephyr's
`native_sim` board, so no hardware is needed.

---

## About the Course
**The hard part of Zephyr is not the kernel.** An engineer who has used FreeRTOS, or written a
scheduler, already knows what a thread, a mutex and a message queue are. What stops people is
everything around them: a workspace that west manages, a build that is CMake plus Kconfig plus
devicetree, and a driver model held together by macros. When Zephyr goes wrong, it is rarely
`k_thread_create()` at fault. It is `undefined reference to __device_dts_ord_42`, or a build aborted
by a Kconfig warning, or a misspelled `CONFIG_` macro that C quietly treats as off.

So the course spends its first two lectures on the build and on devicetree, teaches the kernel as
*the Zephyr way to do what you already know*, and finishes by writing a driver for a sensor that
does not exist, then testing and debugging it.

Topics include:
* The west workspace, the manifest, and pinning a Zephyr release.
* The build: CMake, `prj.conf`, Kconfig, and what to do when the build rejects an option.
* Logging.
* Devicetree: nodes, properties, bindings, overlays, and the macros that read them at compile time.
* Getting devices from the devicetree, and GPIO through Zephyr's emulated GPIO controller.
* Threads, priorities, cooperative and preemptive scheduling, and time.
* Interrupt context, and deferring work out of it with work queues.
* Semaphores, mutexes with priority inheritance, message queues, events and polling.
* Application architecture with zbus and the State Machine Framework.
* The device driver model: API structs, instances, init levels and bindings.
* Emulators, which stand in for hardware in drivers and tests.
* Testing with ztest and Twister.
* The shell, settings stored on a simulated flash, and networking through host sockets.
* Debugging on the host: GDB, sanitizers, fatal error handling and coverage.

---

## Why native_sim
`native_sim` compiles Zephyr, kernel and application together, into an ordinary Linux executable.
There is no instruction set simulator: your code runs natively on the host CPU, and the hardware
around it (a timer, an interrupt controller, a GPIO controller, an I2C bus, a flash chip) is
simulated. The course uses it for three reasons.

**Everything builds and runs on a laptop.** No boards to hand out, no probes, no drivers for a USB
debugger in WSL. A build takes seconds and a run takes less.

**It is deterministic.** Time on `native_sim` is simulated. A program that prints the uptime prints
the same numbers on every run and on every machine, which is what lets the exercises compare your
output with the expected output line by line.

**The host's tools work on it.** GDB, AddressSanitizer, UndefinedBehaviorSanitizer, gcov and
Valgrind all work on a `native_sim` program as on any other Linux program.

The counterweight belongs on the first page: **`native_sim` is not a microcontroller.** Code runs
in zero simulated time, so an interrupt cannot preempt a running thread until that thread calls
into the kernel. A busy loop waiting for an interrupt never ends. Nothing here teaches pin
multiplexing, clocks, power or flashing. L03 shows the time model on purpose, because knowing what
the simulator does *not* do is as important as knowing what it does. Moving the same code onto a
board is the business of **Bootloaders and the Boot Path** and **Hardware-in-the-Loop Testing**.

---

## During the Course
During the lectures participants will:
* Set up a pinned Zephyr workspace and build and run applications on `native_sim`.
* Configure applications with Kconfig and describe hardware with devicetree overlays.
* Write multi-threaded applications with the kernel's scheduling and IPC primitives.
* Structure an application around zbus channels and a state machine.
* Write a devicetree-driven sensor driver, and an emulator to test it against.
* Test with ztest and Twister, and debug with GDB and the sanitizers.

Examples and exercises focus on typical embedded problems, such as:
* Blinking an LED and reading a button, through an emulated GPIO controller.
* Periodic sampling with timers and work queues.
* Producer and consumer threads.
* Priority inversion, produced and then fixed.
* A driver for an I2C temperature sensor.
* A shell command and a setting that survives a restart.

---

## Learning Outcomes
After completing the course, participants should be able to:
* Set up a Zephyr workspace and explain what west, CMake, Kconfig and devicetree each contribute.
* Find out why a configuration option or devicetree node did not take effect.
* Read a devicetree and binding, and access devices and properties from C.
* Choose between cooperative and preemptive threads, and between an ISR, a work item and a thread.
* Use Zephyr's IPC primitives, and explain when each one is the right tool.
* Write a devicetree-instantiated driver for Zephyr's device model.
* Test drivers and applications with ztest, emulators and Twister.
* Explain what `native_sim` simulates faithfully and what it does not.

---

## Checking Your Work
Every lecture has a test suite for its exercises. Clone the repository and set it up:

```bash
git clone https://github.com/qrtech-academy/zephyr.git
cd zephyr
make setup
```

`make setup` creates a Python environment in `.venv`, installs west into it, and fetches the pinned
Zephyr release into `deps/`. [L01 Appendix A](./lectures/L01/appendix/a_setup.md) lists the Ubuntu
packages it needs first. Then, in every new terminal, `source env.sh` before using west.

Write your solutions in `lectures/LNN/exercises`, where each lecture's README says which file goes
where, and run:

```bash
make test               # Every lecture's tests, against your code.
make test SOLUTIONS=1   # The same tests, against the reference solutions.
```

The reference solutions are published in each lecture's `appendix/solutions` a few days after the
lecture. Until then, `make test SOLUTIONS=1` reports that lecture's exercise sets as `SKIP`.

An exercise set you have not started is reported as `SKIP`, with the file its tests are waiting
for; once that file exists, its tests run. Where an exercise gives example output, the tests
compare your program's output with it, line by line.

The lecture examples are checked the way Zephyr checks its own samples, with Twister:

```bash
make build              # Build every lecture example.
make examples           # Build and run every example, and check its output.
```

---

## Two Written Papers, and What They Are Not
Nothing in this course is marked. Assessment is the exercise set after every lecture, each with a
published solution, and the Evaluation questions that close every lecture README.

[`exam/`](./exam/README.md) holds two three-hour papers with worked solutions, and they check
something else: **what you can reconstruct on paper, with nothing in front of you.** Eight questions
each, mixing what a build decided and why with the artifacts a Zephyr application is made of, and no
build in the room to tell you whether you are right.

**They exist purely so that participants can test their own knowledge after the course. They gate
nothing, they are not a qualification, and no part of the course requires them.** Nothing in this
repository depends on them, and neither `make examples` nor `make test` knows they exist.

**Take one after the course is over.** Both papers draw on all six lectures, so sitting one partway
through examines material nobody has taught you yet.

---

## The Book
The whole course is also available as a book: [Zephyr RTOS](./book/zephyr-rtos.pdf). It is built
from the sources in [`book/`](./book/README.md), which also say how to build it yourself
(`make -C book`) and how a new edition is released.

---

## Structure

```text
ci/          Setup, build, test and format scripts
book/        The course typeset as a book; `make -C book` builds the PDF
info/        Course info
lectures/    Lecture plans, examples, exercises, and each lecture's test suite
exam/        Two written papers and their solutions. Optional, and marked by nobody here.
west.yml     The west manifest: which Zephyr release the course is pinned to
env.sh       Sourced in every terminal: activates west and selects the host compiler
deps/        Zephyr, fetched by `make setup`. Not part of the repository.
build-ci/    Output of `make build`, `make examples` and `make test`. Not part of the repository.
```

---

## License
The source code is released under the [MIT License](./LICENSE): the example programs and the
exercise solutions, the test suites and their support code, the west manifest, and the build and
CI scripts.

The course material is licensed under [CC BY-NC-SA 4.0](./LICENSE-CONTENT): the lectures,
exercises, the other Markdown documents, the figures, and the book in `book/`. You may share and
adapt it for any non-commercial purpose, with credit, as long as what you share carries the same
license. The code examples printed in the lectures and in the book may also be used under the MIT
License. Zephyr itself, fetched
into `deps/`, carries its own licenses.

---
