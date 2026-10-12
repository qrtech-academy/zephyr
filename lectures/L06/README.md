# L06 - Subsystems and Debugging

## Agenda
* The shell: built-in commands, commands of your own with subcommands and arguments, and changing
  log levels at run time.
* The settings subsystem: key-value pairs that survive a restart, stored with ZMS on flash.
* `native_sim`'s simulated flash: a file, and the command-line options that control it.
* Networking on `native_sim` through offloaded sockets: Zephyr's socket API on the host's sockets.
* Fatal errors: `k_oops()`, `k_panic()`, assertions, and `k_sys_fatal_error_handler()`.
* The host's tools on a Zephyr program: AddressSanitizer and UndefinedBehaviorSanitizer.
* Debugging a `native_sim` program with GDB.
* Code coverage with gcov and Twister.
* What moving to real hardware would change, and what it would not.

---

## Objectives
After this lecture, participants should:
* Be able to add shell commands to an application, and use the shell to inspect a running system.
* Be able to store and load settings, and understand where they are stored on `native_sim`.
* Be able to write TCP and UDP servers with Zephyr's socket API, and reach them from the host.
* Understand how Zephyr handles a fatal error, and be able to decide what happens after one.
* Be able to find memory errors and undefined behaviour with the sanitizers.
* Be able to debug a `native_sim` program with GDB, and measure test coverage.

---

## Prerequisites
* L01 to L05.
* For the GDB and coverage parts, `gdb` and `lcov` from Ubuntu's packages.

---

## Instructions

### Preparation
* Read [Appendix A](./appendix/a_subsystems.md) and [Appendix B](./appendix/b_debugging.md).
* For further reading, the Zephyr documentation's
  [Shell](https://docs.zephyrproject.org/4.4.0/services/shell/index.html),
  [Settings](https://docs.zephyrproject.org/4.4.0/services/settings/index.html) and
  [Fatal Errors](https://docs.zephyrproject.org/4.4.0/kernel/services/other/fatal.html).

### During the Lecture
* Participate in the lecture (live coding session). The examples are in
  [`examples/`](./examples):
    * [`shell`](./examples/shell/README.md): commands of your own, the kernel's commands, and run-time
      log levels, on the terminal's stdin and stdout.
    * [`settings`](./examples/settings/README.md): a boot counter that survives a restart, on the
      simulated flash.
    * [`sockets`](./examples/sockets/README.md): a TCP echo server on a real port of the host, and a
      self-test that talks to it.
    * [`debugging`](./examples/debugging/README.md): five bugs, caught by AddressSanitizer,
      UndefinedBehaviorSanitizer, and the kernel's fatal error handling; and GDB and coverage.

### After the Lecture
* Complete the exercises in [Appendix C](./appendix/c_exercises.md).
* Write them in [`exercises/`](./exercises/README.md) and check them with `make test` from the
  repository root.
* Exercise solutions are published in `appendix/solutions` a few days after the lecture.
* For further reading, [BSD Sockets](https://docs.zephyrproject.org/4.4.0/connectivity/networking/api/sockets.html)
  and the [native_sim board](https://docs.zephyrproject.org/4.4.0/boards/native/native_sim/doc/index.html)
  documentation, for the peripherals and command-line options this course did not use.

---

## Evaluation
Participants should be able to explain:
* What does a shell command handler receive, and how does it report an error?
* Why can `log enable dbg` not show debug messages from a module compiled at level `INF`?
* Where does the settings subsystem store its data on `native_sim`, and what makes a run a first
  boot?
* What does the offloaded sockets driver do, and why does it need no network setup or root?
* What is the difference between `k_oops()` and `k_panic()`, and what does
  `k_sys_fatal_error_handler()` decide?
* Why does AddressSanitizer work on `native_sim`, and why is it not available when the same code is
  built for a board?
* Why does a UBSan report not stop the program, while an ASan report does?
* What would change, and what would not, if the L05 driver and these subsystems moved to a real
  board?

---

## Where to Go From Here
The course is complete. Everything in it was built for `native_sim`, and every program in it builds
for a real board as it stands.

**Try it on hardware.** Any board Zephyr supports will do; two cheap ones its tree already knows are
an **STM32 Nucleo** (`-b nucleo_f401re`) and an **ESP32 DevKitC**
(`-b esp32_devkitc/esp32/procpu`). Three things change in the workspace:
* **A cross-compiler.** `west sdk install` fetches the Zephyr SDK, which the host compiler stood in
  for until now.
* **The vendor's HAL.** `native_sim` needed no Zephyr modules, so [`west.yml`](../../west.yml)
  imports none; a real SoC needs its own, such as `hal_stm32` or `hal_espressif`. Add an `import`
  with the modules the board needs and run `west update`.
* **Flashing and debugging.** `west flash` and `west debug` replace `west build -t run`: over the
  Nucleo's on-board ST-LINK, or over the DevKitC's USB port.

**What changes on a board**: pins and clocks have to be configured (`pinctrl`), the console is a
real UART, logging is deferred rather than immediate, interrupts arrive while your code runs, stack
overflows can be caught in hardware, and timing is real rather than simulated, so a busy loop wastes
power instead of stopping the world.

**What does not change**: the kernel API, devicetree and Kconfig, the driver model, and your tests.
Twister runs the same ztest suites on the board itself with `--device-testing`, which is the point
of having written them against an emulator.

**Further reading.** Zephyr's documentation lists every supported board, and its `samples/`
directory is the best catalogue of subsystems this course did not cover: Bluetooth, USB, file
systems, power management and the rest.

---
