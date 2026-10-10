# Zephyr RTOS - Written Examination, Paper A

**Time:** 3 hours. **Closed book.** No compiler and no reference material. **Total: 100 marks.**

This paper exists to test your own skills and knowledge. It is not a qualification and it gates
nothing. It draws on all six lectures, so **it is meant to be taken once the course is over**.

---

## Rubric

**Zephyr 4.4** and the board `native_sim/native/64` are assumed throughout, built with the host's
GCC, as in the course. Headers may be assumed included unless a question asks you to name them.

**Code is marked on semantics, not on syntax.** A missing `#include`, a missing semicolon or a brace
in the wrong column costs nothing. A missing `device_is_ready()` check costs everything.

**Where a question says "in the course's style"**, it means Zephyr's own conventions as the
appendices use them: `snake_case`, the kernel's `k_` and `K_` spellings, application options
prefixed `APP_`, `IS_ENABLED()` rather than `#ifdef` where both would work, and log modules rather
than `printk()` in anything that is not a one-off.

**The simulator's behaviour may be assumed** without deriving it: 100 ticks per second, a sleep that
ends on the tick after the one it asked for, simulated time that advances only when every thread is
waiting, immediate logging, and no thread analyzer, stack protection or userspace.

**Where a question says "state the consequence"**, naming the cause earns half the marks and saying
what it does to the running program earns the other half. "This is wrong" scores nothing.

**Where a question asks you to find defects**, the number of defects is stated. Listing more than
the stated number is not penalised, but only the stated number is marked, so put your strongest
answers first.

### Marks

| Question | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   |
| -------- | --- | --- | --- | --- | --- | --- | --- | --- |
| Marks    | 12  | 12  | 13  | 13  | 12  | 13  | 13  | 12  |

---

## Question 1 - The workspace and the build (12 marks)

**(a)** A colleague clones the course repository, runs `make setup`, waits for it to finish, and
then builds:

```text
-- Application: /home/dev/zephyr/lectures/L01/examples/hello
-- ZEPHYR_TOOLCHAIN_VARIANT not set, trying to locate Zephyr SDK
CMake Error at .../cmake/modules/FindZephyr-sdk.cmake:160 (find_package):
  Could not find a package configuration file provided by "Zephyr-sdk"
```

Explain what the build is looking for and why it is not there. State the one command that fixes it,
what that command sets, and why this course needs no Zephyr SDK at all. (3 marks)

**(b)** With that fixed, a build of a new application of their own fails during configuration:

```text
CMake Error at .../cmake/modules/extensions.cmake:3016 (message):
  No prj.conf file(s) was found in the /home/dev/blink folder(s), please read
  the Zephyr documentation on application development.
```

Name the three files every Zephyr application must have and state what each one contributes. Then
state which line of `CMakeLists.txt` does most of the work, and why `project()` must come after it.
(3 marks)

**(c)** The course's `west.yml` holds:

```yaml
manifest:
  remotes:
    - name: zephyrproject-rtos
      url-base: https://github.com/zephyrproject-rtos

  projects:
    - name: zephyr
      remote: zephyrproject-rtos
      revision: v4.4.2
      path: deps/zephyr
      clone-depth: 1
      west-commands: scripts/west-commands.yml

  self:
    path: .
```

State what `west update` does with this file, and where Zephyr ends up. State what would change if
`west-commands` were removed. Then state the two steps that move the course to a newer Zephyr
release, and why the manifest does not import Zephyr's own. (4 marks)

**(d)** A colleague builds once with an option on the command line:

```bash
west build -p -b native_sim/native/64 app -- -DCONFIG_LOG_DEFAULT_LEVEL=2
```

Later they build the same application in the same directory without that option, and the level is
still 2. Explain why, and give the command that proves what the configuration really is. (2 marks)

---

## Question 2 - Kconfig and logging (12 marks)

**(a)** An application adds `CONFIG_SHELL_LOG_BACKEND=y` to its `prj.conf`. The build stops:

```text
warning: SHELL_LOG_BACKEND (defined at subsys/shell/Kconfig:278) was assigned the value 'y' but got
the value 'n'. Check these unsatisfied dependencies: SHELL (=n). ...

error: Aborting due to Kconfig warnings
```

Explain what Kconfig did with the assignment and why Zephyr stops the build rather than carrying on.
Give the two ways of making the build succeed, and state where you would read the value the build
actually ended up with. (4 marks)

**(b)** An application has a Kconfig option `APP_FAHRENHEIT`. Its `main.c` contains:

```c
if (IS_ENABLED(CONFIG_APP_FARENHEIT)) {
	printk("%d F\n", celsius_to_fahrenheit(reading));
}
```

The build succeeds with no warning, and the program prints Celsius whatever `prj.conf` says. Explain
what the compiler did with that line, state the consequence, and state why the same misspelling in
`prj.conf` would not have survived. Name one thing that would have caught it. (3 marks)

**(c)** A module is registered with `LOG_MODULE_REGISTER(monitor);` and the application logs four
messages at `LOG_ERR`, `LOG_WRN`, `LOG_INF` and `LOG_DBG`. The application is built with:

```bash
west build -p -b native_sim/native/64 app -- -DCONFIG_LOG_DEFAULT_LEVEL=2
```

State which of the four messages are printed. State whether the others are in the binary, and
describe one way to demonstrate your answer without reading the source. (3 marks)

**(d)** `native_sim` builds with `CONFIG_LOG_MODE_IMMEDIATE=y`, while the same application built for
a microcontroller gets deferred logging. State one consequence of each mode for the running program.
(2 marks)

---

## Question 3 - Devicetree, bindings and overlays (13 marks)

**(a)** Two builds of the same application fail in two different ways. The first:

```text
error: '__device_dts_ord_32' undeclared here (not in a function); did you mean '__device_dts_ord_38'?
```

The second:

```text
undefined reference to `__device_dts_ord_32'
```

State what `32` is and where you would look it up. Explain what each of the two messages means about
the node, and give one change that produces the first and one that produces the second. (5 marks)

**(b)** A binding declares one required property and one optional one:

```yaml
compatible: "qacademy,alarm"
include: base.yaml
properties:
  threshold-c:
    type: int
    required: true
  label-text:
    type: string
```

Two overlays are written against it. The first omits `threshold-c`. The second adds
`hysteresis-c = <2>;`, which the binding does not declare, and the C code then reads it with
`DT_PROP(DT_NODELABEL(alarm0), hysteresis_c)`.

State what the build does in each case, and at which stage. Then state why a property that the
binding does not declare is a different kind of mistake from a misspelled `CONFIG_` macro in C.
(4 marks)

**(c)** An overlay describes a button as `gpios = <&gpio0 1 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>`.
State what `GPIO_ACTIVE_LOW` changes about the C code that reads the pin, and state what level
`gpio_emul_input_set()` must be given to simulate a press. (2 marks)

**(d)** A node can be reached in C through its node name, its label and an alias. State which of the
three a portable sample should use and why, and state what `DT_NODELABEL()` is for. (2 marks)

---

## Question 4 - Threads, scheduling and time (13 marks)

**(a)** The trace below is printed by an application whose threads are named with their priorities
in brackets. Every thread waits on a semaphore until it is woken.

```text
-- Act 1
main   ( 10)  waking coop_a
coop_a ( -1)  running
coop_a ( -1)  gave coop_b's semaphore: coop_b has a higher priority, but I keep the CPU
coop_b ( -2)  running, because coop_a yielded
coop_a ( -1)  back again, after coop_b returned
-- Act 2
main   ( 10)  waking low
low    (  5)  running
low    (  5)  giving high's semaphore
high   (  2)  running, in the middle of low's k_sem_give()
low    (  5)  back again: high preempted me inside k_sem_give()
```

Explain why `coop_b` does not run at the moment its semaphore is given, while `high` does. State
what must have happened between the third and fourth lines of Act 1. State which priorities are
cooperative and which are preemptible, and what that means for a critical section. (6 marks)

**(b)** On `native_sim`, a thread that calls `k_msleep(500)` at time 0 wakes at 510 ms, but a
one-shot `k_timer` started at time 0 with `K_MSEC(500)` expires at exactly 500 ms. Explain both
numbers. State what a periodic timer does about the difference. (4 marks)

**(c)** An application's `main()` prints one line and returns. The program does not exit. Explain
what returning from `main()` ends and what it does not, state how you would make the program stop by
itself, and state what `-no-rt` changes about the run and what it does not. (3 marks)

---

## Question 5 - Interrupt context, work queues and simulated time (12 marks)

**(a)** The following is submitted for review.

```c
static void sample_expiry(struct k_timer *timer)
{
	struct sample sample;

	LOG_INF("sampling");
	k_mutex_lock(&bus_mutex, K_FOREVER);
	read_sensor(&sample);
	k_mutex_unlock(&bus_mutex);
	k_msleep(5);
	store(&sample);
}

K_TIMER_DEFINE(sample_timer, sample_expiry, NULL);
```

Identify **three** defects, state the consequence of each, and describe the standard Zephyr shape
this function should have instead. (5 marks)

**(b)** The system work queue runs at a cooperative priority. State what happens to other work items
when one of them takes 200 ms, state what happens if a work item blocks forever, and give the one
condition under which you would start a work queue of your own. (3 marks)

**(c)** This loop works on a microcontroller and hangs on `native_sim`:

```c
static volatile bool ready;

static void isr_handler(const struct device *dev, ...)
{
	ready = true;
}

int main(void)
{
	start_hardware();
	while (!ready) {
	}
	LOG_INF("ready");
	return 0;
}
```

Explain what `native_sim` does here and why, state what the same code costs on real hardware, and
give two different rewrites that are correct on both. (4 marks)

---

## Question 6 - Synchronization and communication (13 marks)

**(a)** One application is run twice. Three threads share one CPU: `low` (priority 7) takes a lock
and computes for 100 ms, `high` (priority 5) wants the same lock 21 ms later, and `medium`
(priority 6) wakes at 31 ms and computes for 500 ms without needing the lock.

With a semaphore as the lock:

```text
[   0 ms] low    takes the semaphore
[  21 ms] high   wants the semaphore
[  31 ms] medium starts computing, needs no lock
[ 531 ms] medium is done
[ 600 ms] low    gives the semaphore back
high waited 579 ms for the semaphore
```

With a `k_mutex` instead:

```text
[   0 ms] low    takes the mutex
[  21 ms] high   wants the mutex
[ 100 ms] low    (priority 5) gives the mutex back
high waited 79 ms for the mutex
```

Name the phenomenon the first trace shows. Explain both waiting times, arithmetic included. State
what the kernel did to `low` in the second run and when it undid it. State one thing the mutex does
**not** fix. (6 marks)

**(b)** A producer submits samples to a `k_msgq` with `k_msgq_put(&q, &sample, K_NO_WAIT)` and the
call starts returning `-ENOMSG`. State what that means and what happens to the sample. State what
`K_FOREVER` would do instead, and which of the two an interrupt handler may use. (3 marks)

**(c)** Three observers are attached to one zbus channel: a listener, a subscriber and a message
subscriber. Over a run of several publications, the subscriber prints one reading twice and never
prints another, while the message subscriber prints every reading exactly once. Explain the
subscriber's behaviour. State in which thread and with what held the listener runs, and what that
forbids it from doing. (4 marks)

---

## Question 7 - Writing a driver (13 marks)

**(a)** An application starts with:

```c
const struct device *const sensor = DEVICE_DT_GET(DT_NODELABEL(thermo0));

if (!device_is_ready(sensor)) {
	LOG_ERR("thermo0 is not ready");
	return 0;
}
```

The message is printed on every boot, although the build succeeded and the node is enabled. State
the two things `device_is_ready()` reports on, name the most likely thing the driver's init function
did, and state what would happen if the application used the device anyway. (4 marks)

**(b)** A driver for an I2C part contains the following. It builds, and one instance works until a
second is added to the devicetree.

```c
#define DT_DRV_COMPAT qacademy_thermo

struct thermo_data {
	struct i2c_dt_spec i2c;
	int16_t last_sample;
};

static int thermo_init(const struct device *dev)
{
	struct thermo_data *data = dev->data;
	uint8_t id;

	data->i2c.bus = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	data->i2c.addr = 0x48;

	(void)i2c_reg_read_byte_dt(&data->i2c, THERMO_REG_WHO_AM_I, &id);
	return 0;
}
```

Identify **three** defects, state the consequence of each, and give the corrected shape of the
affected lines. (5 marks)

**(c)** The driver's Kconfig option is written `depends on DT_HAS_QACADEMY_THERMO_ENABLED`. State
where that symbol comes from and what the dependency achieves. State what happens to the driver, and
to an application that calls `DEVICE_DT_GET()` on the node, if the node is given
`status = "disabled"`. (2 marks)

**(d)** On this build, I2C controllers initialize at priority 50 and sensors at 90, both at the
`POST_KERNEL` level. State what the numbers order, and what would go wrong if the sensor's priority
were 40. (2 marks)

---

## Question 8 - Subsystems and debugging (12 marks)

**(a)** A shell session on a running program:

```text
uart:~$ greet Ada 9
times must be between 1 and 5, not '9'
uart:~$ retval
-22
uart:~$ foo
foo: command not found
uart:~$ retval
-8
uart:~$ led
led - control the LED
Subcommands:
  on  off  status
uart:~$ retval
1
```

State where each of the three return values comes from, and what each one means. State what the
shell does with a handler's return value, and why a command handler should return a negative errno
rather than printing and returning 0. (3 marks)

**(b)** A boot counter is stored with the settings subsystem. It counts 1, 2, 3 over three runs
started with `west build -t run`, then goes back to 1 after the developer rebuilds with `-p`.
Explain where the counter lives on `native_sim` and why a pristine build resets it. Name the
command-line option that makes every run a first boot without deleting anything. (3 marks)

**(c)** Two runs of the same application end differently:

```text
[00:00:00.000,000] <err> os: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
[00:00:00.000,000] <err> app: main() is still running, and returns
```

```text
ASSERTION FAIL [percent <= 100] @ .../src/main.c:41
[00:00:00.000,000] <err> os: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
Exiting due to fatal error
```

Explain the difference between the two reasons, state what the application's
`k_sys_fatal_error_handler()` must do in each case, and state what the process exit status is in the
second. (3 marks)

**(d)** A build with `CONFIG_ASAN=y` prints:

```text
==1354852==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x78a35ddf4070
WRITE of size 1 at 0x78a35ddf4070 thread T2
    #0 0x402bbe in fill .../src/main.c:28
```

State what the report tells you that a microcontroller would not have told you at all. State why the
program stops here rather than continuing, and state why AddressSanitizer is available on this board
and not on a Cortex-M build of the same application. (3 marks)

---
