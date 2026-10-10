# Course Information

## Instructor
Erik Pihl ([erik.axel.pihl@gmail.com](mailto:erik.axel.pihl@gmail.com))

---

# Course Plan – Zephyr RTOS

| Week | Lecture | Topic |
|------|---------|------|
| TBD | L01 | Zephyr, west and the build |
| TBD | L02 | Devicetree |
| TBD | L03 | Kernel I: threads, time and work queues |
| TBD | L04 | Kernel II: communication and application architecture |
| TBD | L05 | Writing a driver |
| TBD | L06 | Subsystems and debugging |

---

## Lecture Content

### L01 – Zephyr, west and the Build
What Zephyr is, and how an application becomes a program.

Topics include:
* Zephyr's parts: kernel, driver model, subsystems, build system and test tooling
* `native_sim`, and simulated time
* The west workspace and manifest
* Application structure: `CMakeLists.txt`, `prj.conf`, `src/`
* `west build`, the build directory, and running on `native_sim`
* Kconfig: options, dependencies, where values come from, `IS_ENABLED()`
* Application-specific Kconfig options
* `printk()` and the logging subsystem

---

### L02 – Devicetree
Describing hardware, and using it from C.

Topics include:
* Nodes, properties, labels, phandles, aliases and `chosen`
* Board devicetrees and overlays
* Bindings
* Devicetree macros in C
* Getting devices, and `device_is_ready()`
* GPIO outputs, inputs and interrupts on the emulated GPIO controller
* Devicetree build errors

---

### L03 – Kernel I: Threads, Time and Work Queues
Scheduling, the Zephyr way.

Topics include:
* Threads and priorities
* Cooperative and preemptive scheduling, and `k_yield()`
* Ticks, timeouts and sleeping
* Kernel timers and interrupt context
* Work queues and delayable work
* `native_sim`'s time model, and busy loops
* Static stack usage analysis

---

### L04 – Kernel II: Communication and Application Architecture
Getting threads to work together.

Topics include:
* Semaphores, mutexes and events
* Priority inversion and priority inheritance
* Message queues and `k_poll()`
* Memory without `malloc()`
* zbus
* The State Machine Framework

---

### L05 – Writing a Driver
A driver for a part that does not exist.

Topics include:
* The device driver model
* Devicetree-instantiated drivers, and init levels
* Bindings and Kconfig for a new part
* The sensor API
* Zephyr modules
* Emulators
* ztest and Twister

---

### L06 – Subsystems and Debugging
Making a program talk back, remember things, and crash usefully.

Topics include:
* The shell and custom commands
* Settings on the simulated flash
* Networking through host sockets
* Fatal errors and assertions
* AddressSanitizer and UndefinedBehaviorSanitizer
* Debugging `native_sim` programs with GDB
* Code coverage

---

## Course Material

### Literature
The course material consists of:
* Lecture notes
* Code examples
* Exercises completed after the lectures

---

### Software
Recommended tools:
* **[Visual Studio Code](https://code.visualstudio.com/download)** – Primary editor for writing and exploring code examples
* **Linux / WSL 2** – Every program in the course builds and runs on Linux
* **GCC and CMake** – Used to build Zephyr applications for `native_sim`
* **west** – Zephyr's workspace and build tool, installed by `make setup`

Instructions for installing the development environment are available [here](../lectures/L01/appendix/a_setup.md).

---
