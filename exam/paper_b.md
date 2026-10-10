# Zephyr RTOS - Written Examination, Paper B

**Time:** 3 hours. **Closed book.** No compiler, no workspace and no reference material.
**Total: 100 marks.**

This paper exists to test your own skills and knowledge. It is not a qualification and it gates
nothing. It draws on all six lectures, so **it is meant to be taken once the course is over**.

---

## Rubric

**Zephyr 4.4 and the board `native_sim/native/64`** are assumed throughout, built with the host's
GCC, as in the course. Headers may be assumed included unless a question asks you to name them.

**Code is marked on semantics, not on syntax.** A missing `#include`, a missing semicolon or a brace
in the wrong column costs nothing. A missing `device_is_ready()` check costs everything.

**Where a question says "in the course's style"**, it means the conventions used in the appendices:

* Zephyr's own C style: `snake_case`, the kernel's `k_` and `K_` spellings.
* Application options prefixed `APP_`, and read with `IS_ENABLED()` rather than `#ifdef` wherever
  both would work.
* Log modules rather than `printk()` in anything that is not a one-off.
* Hardware taken from the devicetree, never a pin number written into the C.

**The simulator's behaviour may be assumed** without deriving it: 100 ticks per second, a sleep that
ends on the tick after the one it asked for, simulated time that advances only when every thread is
waiting, immediate logging, and no thread analyzer, stack protection or userspace.

**Where a question says "state the consequence"**, naming a cause earns half the marks and saying
what it does to the running program earns the other half. "This is wrong" scores nothing.

**Where a question asks you to write code**, documentation comments are not required and are not
marked. Write the code.

### Marks

| Question | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   |
| -------- | --- | --- | --- | --- | --- | --- | --- | --- |
| Marks    | 12  | 12  | 13  | 13  | 12  | 13  | 13  | 12  |

---

## Question 1 - The workspace and the build (12 marks)

**(a)** Write the `west.yml` of a workspace that pins Zephyr 4.4.2, checks it out into `deps/zephyr`,
fetches only the pinned commit rather than the project's history, makes Zephyr's own west commands
available, and treats the repository holding the manifest as the workspace itself. Then state what
`west update` does, and name one thing it does **not** do. (4 marks)

**(b)** Write the three files of the smallest application that logs `ready` at level INF:
`CMakeLists.txt`, `prj.conf` and `src/main.c`. State what `find_package(Zephyr)` does, and why
`project()` must come after it. (4 marks)

**(c)** Write the command that builds that application for `native_sim/native/64` from the
repository root, discarding any earlier build, and the command that then runs it. Write the command
that builds the same application with an extra configuration file `debug.conf` merged on top of
`prj.conf`.

Then name three files under `build/` that record what the build decided, and state what each holds.
(4 marks)

---

## Question 2 - Configuring an application (12 marks)

**(a)** An application samples a temperature and can warn when it gets too hot. Write its `Kconfig`
file, declaring:

* `APP_SAMPLE_PERIOD_MS`, an integer number of milliseconds between samples, between 10 and 10000,
  1000 unless set;
* `APP_ALARM`, off unless set, which turns the warning on;
* `APP_ALARM_LIMIT_C`, an integer with the default 30, which exists only when the alarm does.

Write the `prj.conf` that samples every 250 ms with the alarm on. Then state what the build does if
a `prj.conf` sets `CONFIG_APP_ALARM_LIMIT_C=35` while leaving the alarm off. (5 marks)

**(b)** Write the C that logs each reading through a log module named `sampler` and, when the alarm
is enabled, warns about a reading at or above the limit. The warning lives in `src/alarm.c`, which
is compiled only when the alarm is enabled; write the line in `CMakeLists.txt` that arranges that,
and write the call site in `src/main.c`.

State why the call site uses `IS_ENABLED()` rather than `#ifdef`, and what the compiler does with
the branch that cannot run. (4 marks)

**(c)** Name, in the order the build applies them, four places a Kconfig option's value can come
from, and state which one wins.

Then: a build directory is created with `-DCONFIG_APP_SAMPLE_PERIOD_MS=50` on the command line.
Later the same directory is built again, without `-p` and without that option. State which period
that build uses, and why. (3 marks)

---

## Question 3 - Describing hardware (13 marks)

A product has heaters, each switched by one GPIO pin. They are not on a bus, and there is no
existing binding for them.

**(a)** Write the binding `qacademy,heater`. A node shall have a required `control-gpios` giving the
pin that switches the heater, a required string `zone` naming the area it warms, and an integer
`max-power-percent` which is 100 unless the node says otherwise.

State where in an application the file goes, and what else the build needs before it will accept
the vendor prefix `qacademy` without complaint. (5 marks)

**(b)** Write the overlay that adds two of them: `heater_a` in zone `cabin` on `gpio0` pin 2, active
high; `heater_b` in zone `tank` on `gpio0` pin 3, active low, limited to 60 %. Give the first the
alias `primary-heater`.

State the file name that makes the build pick the overlay up without being told, and state what the
overlay does to the board's own devicetree. (4 marks)

**(c)** Write the C that builds a table of every enabled heater **at compile time**, holding for each
one its GPIO specification, its zone and its power limit, and then configures each pin as an
inactive output. Use the table to log each heater's zone and pin.

State what changes in that table if `heater_b` is given `status = "disabled"`, and state what
happens instead if its `zone` property is deleted. (4 marks)

---

## Question 4 - Threads and time (13 marks)

A system has three activities:

1. a control loop that must run every 10 ms and must not be late;
2. a display that is refreshed every 200 ms;
3. a logger that writes to flash, takes up to 50 ms, and may be late.

**(a)** Choose a priority for each, say for each whether it is cooperative or preemptible, and
justify the three choices in terms of what happens when two of them are ready at once. Write the
three `K_THREAD_DEFINE()` lines. (5 marks)

**(b)** Write the control loop's body so that it runs every 10 ms without drift, and explain why a
loop whose body ends with `k_msleep(10)` does drift. Name a second way to get the same result, and
state what makes it equivalent. (4 marks)

**(c)** On `native_sim`, `k_msleep(30)` returns after 40 ms, while a periodic timer started with
`k_timer_start(&t, K_MSEC(30), K_MSEC(30))` expires at 30 ms, 60 ms and 90 ms. Explain both.

Then state what `CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000` changes about the first, and name one cost of
setting it. (4 marks)

---

## Question 5 - Interrupts, work and the simulator (12 marks)

**(a)** Write a sampler that takes a reading every `CONFIG_APP_SAMPLE_PERIOD_MS` milliseconds: a
`k_timer` whose expiry function hands the reading over to the system work queue, and a work handler
that logs it.

Name two things the expiry function must not do, and state for each what would happen if it did.
State which thread the work handler runs in. (5 marks)

**(b)** Extend it: if no sample has been taken for 250 ms, log `idle`. Write the code, and state why
it uses `k_work_reschedule()` rather than `k_work_schedule()`.

State, to the nearest tick, when the message is logged if the last sample was taken at 500 ms.
(4 marks)

**(c)** A colleague waits for the first sample like this, with `ready` set by the timer's expiry
function:

```c
static volatile bool ready;

while (!ready) {
}
```

State what this program does on `native_sim`, and why. Rewrite the wait correctly. (3 marks)

---

## Question 6 - Threads that must agree (13 marks)

**(a)** A sampler thread produces a `struct sample` every 100 ms; a storage thread takes 250 ms to
write each one away. Write the message queue between them, the producer's put and the consumer's
get, so that the sampler never waits and a sample that does not fit is counted as dropped.

State what `k_msgq_put()` does with the struct, and why that means the producer's local variable may
go out of scope immediately. (5 marks)

**(b)** A shared calibration table is guarded by a semaphore created with `K_SEM_DEFINE(lock, 1, 1)`.
Rewrite the take and the give with the primitive the course would use instead, and state the two
things it gives you that the semaphore does not.

Then: three threads, at priorities 5, 6 and 7, use the CPU; the one at 7 holds the guard and the one
at 5 wants it. Name what the semaphore version suffers from, and state what the replacement bounds
the highest-priority thread's wait to. (4 marks)

**(c)** The same sample is now wanted by two independent parts of the application. Write a zbus
channel carrying it, with a validator that rejects a reading outside -40 to 125 degrees, one
listener and one subscriber.

State in which thread each of the two observers runs, and explain why a subscriber can miss a
message that a listener cannot. (4 marks)

---

## Question 7 - A driver of your own (13 marks)

Baro is a made-up I2C pressure sensor. Its register map is:

| Register      | Address | Contents                                               |
| ------------- | ------- | ------------------------------------------------------ |
| `WHO_AM_I`    | `0x0F`  | Reads `0x5B`, always.                                  |
| `PRESSURE_L`  | `0x28`  | The low byte of the reading.                           |
| `PRESSURE_H`  | `0x29`  | The high byte. The reading is unsigned, in steps of 10 Pa. |

**(a)** Write the driver, as a devicetree-instantiated driver in Zephyr's device model: the
compatible it serves, its configuration and its run-time data, an initialization function that
refuses a part that does not identify itself, the two sensor API functions that read the pressure,
and the macro that defines one device per devicetree node.

The sensor API reports pressure in kilopascals, so a reading of 10132 counts is `val1` 101 and
`val2` 320000. (6 marks)

**(b)** The driver is shipped as a Zephyr module. Write its `zephyr/module.yml`, the Kconfig option
that turns the driver on, and the binding `qacademy,baro`.

State why the Kconfig option depends on `DT_HAS_QACADEMY_BARO_ENABLED`, and what that arranges for
an application that does not have the part. (4 marks)

**(c)** An emulator for the part provides:

```c
void baro_emul_set_raw(const struct emul *target, uint16_t raw);
```

Write a ztest suite with two tests: that the device is ready, and that 10132 counts is read back as
101.32 kPa. Name the file that tells Twister how to run it.

State what `device_is_ready()` returns for a device whose initialization function returned
`-ENODEV`, and what the application must therefore do. (3 marks)

---

## Question 8 - Making it usable (12 marks)

**(a)** Write the shell command `setpoint`, with two subcommands: `setpoint get`, which prints the
current value, and `setpoint set <celsius>`, which accepts -40 to 125 and rejects anything else with
a message.

State what the shell has already checked before your `set` handler runs, and what the handler's
return value is used for. (5 marks)

**(b)** Make the setpoint survive a restart. Write the settings handler and the call that stores the
value, and name the Kconfig options the application needs.

State where the value is kept on `native_sim`, and name two ways to make the next run behave as a
first boot. (4 marks)

**(c)** Write a fatal error handler that aborts only the offending thread when a thread calls
`k_oops()`, but halts the system for anything else.

State what the process's exit status is on `native_sim` when it halts, and why returning from a
kernel panic is not allowed. (3 marks)

---
