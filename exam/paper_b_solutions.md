# Paper B - Solutions

Marks are shown per part. Method carries them: a correct design with a clumsy fix is worth more than
a correct fix with no reasoning, and several later parts consume earlier answers, so an error should
be followed through rather than penalised twice.

Code is marked on semantics. Missing includes, missing semicolons and misplaced braces cost nothing.

---

## Question 1 - The workspace and the build (12 marks)

### (a) 4 marks

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

*(2 marks: 1 for a manifest with a remote and a pinned project, 1 for `path`, `clone-depth` and
`west-commands` together. `self: path: .` earns its mark under the last bullet below.)*

Credit each of these where the candidate says it:

* **`revision`** may be a tag, a branch or a commit hash; a product pins a tag or a hash, so that
  the same manifest checks out the same code next year.
* **`clone-depth: 1`** fetches only the pinned commit, which is most of the download; the cost is
  that `git log` in `deps/zephyr` shows one commit.
* **`west-commands`** is what makes `west build` and `west twister` exist. Without it west has only
  its own commands, because the course's manifest does not import Zephyr's.
* **`self: path: .`** says the manifest repository *is* the workspace directory.

**What `west update` does:** it checks out every project in the manifest at the revision the
manifest names, cloning it first if it is not there.
*(1 mark)*

**What it does not do:** it does not update the manifest. Nothing fetches a newer Zephyr by itself:
somebody edits `revision`, or a `git pull` of the course repository brings a new manifest in, and
only then does `west update` make the workspace match it. Accept also: it does not build anything,
and it does not touch projects that are already at the right revision.
*(1 mark)*

### (b) 4 marks

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ready)

target_sources(app PRIVATE src/main.c)
```

```text
CONFIG_LOG=y
```

```c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("ready");
	return 0;
}
```

*(2 marks: 1 for the three CMake lines plus `target_sources`, 1 for the module registration and the
log call. `prj.conf` must exist, and `CONFIG_LOG=y` must be in it; an application with no `prj.conf`
fails to configure with `No prj.conf file(s) was found`.)*

**What `find_package(Zephyr)` does.** It is where Zephyr's build system takes over: it finds Zephyr
through `ZEPHYR_BASE`, reads the board, processes the devicetree, runs Kconfig, sets Zephyr up as a
set of CMake libraries, and creates the library target `app` that the application's own sources are
added to.
*(1 mark)*

**Why `project()` comes after it.** Zephyr selects the toolchain during `find_package`, and CMake's
`project()` is what makes a compiler current. Calling `project()` first would have CMake look for a
host compiler before Zephyr had said which compiler to use.
*(1 mark)*

### (c) 4 marks

```bash
west build -p -b native_sim/native/64 app
west build -t run
```

*(1 mark. `-p` is pristine: it deletes the build directory first. Accept `./build/zephyr/zephyr.exe`
in place of `west build -t run`.)*

```bash
west build -p -b native_sim/native/64 app -- -DEXTRA_CONF_FILE=debug.conf
```

*(1 mark. Everything after `--` goes to CMake. `CONF_FILE` in place of `EXTRA_CONF_FILE` is wrong
and earns nothing: it replaces `prj.conf` rather than merging on top of it.)*

**Three files, any three of** *(2 marks)*:

| File | What it holds |
| --- | --- |
| `build/zephyr/.config` | Every Kconfig option and the value the build settled on. |
| `build/zephyr/include/generated/zephyr/autoconf.h` | The same configuration as C macros, included into every source file. |
| `build/zephyr/zephyr.dts` | The final devicetree, overlays applied. |
| `build/zephyr/include/generated/zephyr/devicetree_generated.h` | The devicetree as C macros. |
| `build/zephyr/zephyr.exe` | The program itself, on `native_sim`. |
| `build/zephyr/zephyr.map` | The linker map: where every symbol ended up. |
| `build/compile_commands.json` | Every compiler command line, for editors. |

---

## Question 2 - Configuring an application (12 marks)

### (a) 5 marks

```text
menu "Application"

config APP_SAMPLE_PERIOD_MS
	int "Milliseconds between samples"
	range 10 10000
	default 1000

config APP_ALARM
	bool "Warn when a reading reaches the limit"

config APP_ALARM_LIMIT_C
	int "Temperature at which the alarm warns, in degrees Celsius"
	depends on APP_ALARM
	default 30

endmenu

source "Kconfig.zephyr"
```

*(3 marks: 1 for the two typed options with prompt, range and defaults; 1 for `depends on APP_ALARM`
on the limit; 1 for `source "Kconfig.zephyr"` at the end. An application's `Kconfig` replaces
Zephyr's top-level one, so without that last line not one of Zephyr's own options exists and the
build fails on the board's own defaults, with
`attempt to assign the value 'y' to the undefined symbol CONSOLE`.)*

```text
CONFIG_APP_SAMPLE_PERIOD_MS=250
CONFIG_APP_ALARM=y
```

*(1 mark.)*

**With the alarm off, setting the limit aborts the build.** `APP_ALARM_LIMIT_C` depends on
`APP_ALARM`, so with the alarm off the symbol does not exist and the assignment cannot take. Kconfig
says so, and Zephyr treats the warning as an error:

```text
warning: APP_ALARM_LIMIT_C (defined at Kconfig:11) was assigned the value '35' but got the value ''.
Check these unsatisfied dependencies: APP_ALARM (=n) ...

error: Aborting due to Kconfig warnings
```

The alternative, a build that silently ignores the line, is what the abort exists to prevent.
*(1 mark. Accept an answer that only says "the build aborts because the dependency is not met".)*

### (b) 4 marks

```cmake
target_sources_ifdef(CONFIG_APP_ALARM app PRIVATE src/alarm.c)
```

*(1 mark. Kconfig has run by the time `find_package(Zephyr)` returns, so every option is a CMake
variable too; this is how nearly all of Zephyr is built.)*

```c
LOG_MODULE_REGISTER(sampler, LOG_LEVEL_INF);

/* ... */
	LOG_INF("Reading %d C", celsius);

	if (IS_ENABLED(CONFIG_APP_ALARM)) {
		alarm_check(celsius);
	}
```

with `alarm.c` holding

```c
LOG_MODULE_DECLARE(sampler);

void alarm_check(int celsius)
{
	if (celsius >= CONFIG_APP_ALARM_LIMIT_C) {
		LOG_WRN("Reading %d C is at or above the limit of %d C", celsius,
			CONFIG_APP_ALARM_LIMIT_C);
	}
}
```

*(2 marks: 1 for the module registration and the guarded call, 1 for `LOG_MODULE_DECLARE` in the
second file. A second `LOG_MODULE_REGISTER` of the same name is the mistake here; the module is
registered once and declared everywhere else.)*

**Why `IS_ENABLED()`.** It expands to `1` or `0`, so the call is compiled and type-checked in every
configuration, and the compiler removes the branch that cannot run — call included, which is why the
program still links when `alarm.c` is not compiled. Code behind `#ifdef` is not even parsed when the
option is off, so it rots unnoticed until somebody builds the other configuration.
*(1 mark)*

### (c) 3 marks

**In order** *(2 marks, all four and the right order)*:

1. the option's own `default` in the `Kconfig` file;
2. the board's defaults, from its `_defconfig`;
3. the application's `prj.conf`, and any board-specific `.conf` beside it;
4. extra configuration files given with `-DEXTRA_CONF_FILE=`;
5. `-DCONFIG_...` on the `west build` command line.

**The last one applied wins**, so the command line beats everything else. (Accept any four of these
five in the right order.)

**The second build still uses 50 ms.** A `-DCONFIG_...` option is stored in the build directory's
CMake cache, and every later build in that directory keeps using it until the directory is rebuilt
with `-p` or the option is given a new value. This is one of the ways a build directory goes stale,
and why a pristine build is the only way to be sure that nothing but the files and the command line
decide the configuration.
*(1 mark)*

---

## Question 3 - Describing hardware (13 marks)

### (a) 5 marks

```yaml
description: |
  A heater driven by one GPIO pin.

compatible: "qacademy,heater"

properties:
  control-gpios:
    type: phandle-array
    required: true
    description: The pin that switches the heater on.

  zone:
    type: string
    required: true
    description: The name of the zone this heater warms.

  max-power-percent:
    type: int
    default: 100
    description: The highest duty cycle this heater may be driven at.
```

*(3 marks: 1 for `compatible` and the three property names, 1 for the types, 1 for `required: true`
on two and `default: 100` on the third. A `default` on a required property is a contradiction and
loses the last mark.)*

**Where it goes:** `dts/bindings/` in the application directory — the application's own directory is
searched for bindings, alongside Zephyr's `dts/bindings`. The file name does not matter to the
build, but the convention is the compatible: `dts/bindings/qacademy,heater.yaml`.
*(1 mark)*

**The vendor prefix.** `qacademy` is not one of the prefixes Zephyr knows, so the build warns
`unknown vendor prefix 'qacademy'`. A `dts/bindings/vendor-prefixes.txt` beside the binding, with
the line `qacademy<TAB>QAcademy`, registers it. This matters more than the warning suggests: Twister
builds with `--edtlib-Werror`, where the same warning is an error.
*(1 mark)*

### (b) 4 marks

```dts
/ {
	heater_a: heater-a {
		compatible = "qacademy,heater";
		control-gpios = <&gpio0 2 GPIO_ACTIVE_HIGH>;
		zone = "cabin";
	};

	heater_b: heater-b {
		compatible = "qacademy,heater";
		control-gpios = <&gpio0 3 GPIO_ACTIVE_LOW>;
		zone = "tank";
		max-power-percent = <60>;
	};

	aliases {
		primary-heater = &heater_a;
	};
};
```

*(3 marks: 1 for two nodes with the right compatible at the root, 1 for the `gpios` cells with their
flags, 1 for the alias. `max-power-percent` must be left out of `heater_a`, which is what makes the
binding's default do its work.)*

**The file name is `app.overlay`**, in the application directory: the build picks that up with no
argument and no CMake line. (Accept `boards/native_sim_native_64.overlay` with the note that a
board-named overlay *replaces* `app.overlay` rather than adding to it.)

**What it does:** the overlay is merged on top of the board's devicetree before anything is
generated from it, adding nodes, changing properties, or disabling nodes the board declared. The
board's own file is not edited; the result is what `build/zephyr/zephyr.dts` shows.
*(1 mark)*

### (c) 4 marks

```c
struct heater {
	struct gpio_dt_spec control;
	const char *zone;
	int max_power_percent;
};

#define HEATER_ENTRY(node)                                                                         \
	{                                                                                          \
		.control = GPIO_DT_SPEC_GET(node, control_gpios),                                  \
		.zone = DT_PROP(node, zone),                                                       \
		.max_power_percent = DT_PROP(node, max_power_percent),                             \
	},

static const struct heater heaters[] = {DT_FOREACH_STATUS_OKAY(qacademy_heater, HEATER_ENTRY)};

int main(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(heaters); i++) {
		const struct heater *h = &heaters[i];

		if (!gpio_is_ready_dt(&h->control)) {
			LOG_ERR("%s: controller not ready", h->zone);
			continue;
		}
		gpio_pin_configure_dt(&h->control, GPIO_OUTPUT_INACTIVE);
		LOG_INF("%-6s pin %u, at most %d %%", h->zone, h->control.pin,
			h->max_power_percent);
	}
	return 0;
}
```

*(3 marks: 1 for `DT_FOREACH_STATUS_OKAY` with the compatible spelled in C, commas and dashes
replaced by underscores, and a macro that expands to one initializer including its trailing comma;
1 for `GPIO_DT_SPEC_GET` and `DT_PROP`; 1 for the readiness check and configuring the pin as an
inactive output. The table is `static const`: it is built by the compiler and lives in flash.
Nothing here is a search at run time.)*

**With `status = "disabled"`** the node is not "status okay", so `DT_FOREACH_STATUS_OKAY` does not
expand it: the table has one entry, `ARRAY_SIZE` is 1, and nothing else changes. It still builds.
*(0.5 marks)*

**With `zone` deleted** the build stops during configuration, before any C is compiled:

```text
devicetree error: 'zone' is marked as required in 'properties:' in
'.../dts/bindings/qacademy,heater.yaml', but does not appear in <Node /heater-b in
'.../app.overlay'>
```

That is the binding doing its job: a required property is a promise the devicetree must keep.
*(0.5 marks)*

---

## Question 4 - Threads and time (13 marks)

### (a) 5 marks

A defensible answer, and the one the course leads to:

| Activity | Priority | Kind | Why |
| --- | --- | --- | --- |
| Control loop | `-1` | Cooperative | It must not be late. A cooperative thread cannot be preempted at all, so once it is running nothing else runs until it blocks — which it does, on its next period. |
| Display | `5` | Preemptible | It must not delay the control loop, and it does not mind being interrupted part-way through a refresh. |
| Logger | `7` | Preemptible, lowest | It is the one that may be late. At the lowest priority it runs only when the other two are waiting, and its 50 ms of flash work delays nothing. |

```c
K_THREAD_DEFINE(control, 1024, control_thread, NULL, NULL, NULL, -1, 0, 0);
K_THREAD_DEFINE(display, 1024, display_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(logger, 2048, logger_thread, NULL, NULL, NULL, 7, 0, 0);
```

*(3 marks for the three choices with a reason each; 2 marks for the three definitions. In Zephyr a
**lower number is a higher priority**, and negative priorities are the cooperative ones: getting
that backwards costs the marks for the choices. Any consistent set of preemptible priorities is
fine — what is marked is control above display above logger, and the reasoning.)*

Credit as an addition: the logger is the thread to give the larger stack, because it is the one that
calls into a file system or flash; and a cooperative control loop is only safe because its body is
short and ends in a wait.

### (b) 4 marks

```c
static K_TIMER_DEFINE(control_tick, NULL, NULL);

static void control_thread(void)
{
	k_timer_start(&control_tick, K_MSEC(10), K_MSEC(10));

	while (true) {
		k_timer_status_sync(&control_tick);   /* Waits for the next expiry. */
		control_step();
	}
}
```

*(2 marks. `k_timer_status_sync()` blocks until the timer has expired at least once and returns how
many times it has, so a slow iteration shows up as a count above 1 rather than as drift.)*

**Why `k_msleep(10)` drifts.** It sleeps for 10 ms *from the moment it is called*, which is after
the body has run. Every iteration is therefore the body plus 10 ms plus the rounding to the next
tick, and the error accumulates: the loop's period is never 10 ms and the tenth iteration is late by
ten roundings. A periodic timer measures from the previous expiry, so its errors do not add up.
*(1 mark)*

**A second way:** absolute timeouts. Keep a deadline and sleep until it, rather than for a duration:

```c
int64_t next = k_uptime_get() + 10;

while (true) {
	k_sleep(K_TIMEOUT_ABS_MS(next));
	control_step();
	next += 10;
}
```

This is equivalent because the wake-up time, not the sleep's length, is what is specified: the body's
duration comes out of the wait rather than being added to it.
*(1 mark)*

### (c) 4 marks

**`k_msleep(30)` returns after 40 ms.** The kernel keeps timeouts in ticks, and `native_sim` has 100
of them a second, so 30 ms is three ticks. The thread calls at some point *within* the current tick,
and the kernel cannot know how much of it is left, so it waits the three ticks the duration needs
plus the remainder of the one in progress: four tick boundaries, 40 ms. A sleep is a *minimum*.
*(2 marks)*

**The timer expires at 30, 60 and 90.** `k_timer_start()` compensates for exactly that added tick,
and a periodic timer's next expiry is measured from the previous one rather than from when the
handler ran, so a periodic timer does not drift.
*(1 mark)*

**With 1000 ticks per second**, a tick is 1 ms, so the same sleep returns after 31 ms: the rounding
is still there but is ten times smaller. The cost is that the kernel's clock bookkeeping happens ten
times as often; on a board that is more timer interrupts, more power, and less time asleep.
*(1 mark)*

---

## Question 5 - Interrupts, work and the simulator (12 marks)

### (a) 5 marks

```c
static void sample_work_handler(struct k_work *work);

static K_WORK_DEFINE(sample_work, sample_work_handler);

static void sample_timer_expired(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_work_submit(&sample_work);          /* All an ISR should do: hand the work on. */
}

static K_TIMER_DEFINE(sample_timer, sample_timer_expired, NULL);

static void sample_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	LOG_INF("sample %d", read_sensor());
}

int main(void)
{
	k_timer_start(&sample_timer, K_MSEC(CONFIG_APP_SAMPLE_PERIOD_MS),
		      K_MSEC(CONFIG_APP_SAMPLE_PERIOD_MS));
	return 0;
}
```

*(3 marks: 1 for the timer with a period, 1 for the expiry function doing nothing but submitting,
1 for the work item and its handler.)*

**Two things the expiry function must not do** *(1 mark for both, with consequences)*:

* **Sleep or block** — `k_sleep()`, `k_mutex_lock()` with a timeout, `k_msgq_get(..., K_FOREVER)`.
  A timer's expiry function runs in the system clock's interrupt handler, and there is no thread to
  put to sleep. With `CONFIG_ASSERT=y` the kernel catches it and panics; without, the system
  misbehaves.
* **Take long** — the expiry function delays every interrupt behind it and every thread. Anything
  that reads a sensor over a bus, writes to flash, or formats a long log line belongs in the work
  handler.

**Which thread the handler runs in:** the system work queue's thread, `sysworkq`, a cooperative
thread shared by every subsystem. That is why it may sleep and log, and why it should still be
quick: everybody else's work is queued behind it. (A queue of your own, started with
`k_work_queue_start()`, is the answer when the work is slow.)
*(1 mark)*

### (b) 4 marks

```c
static void idle_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	LOG_INF("idle");
}

static K_WORK_DELAYABLE_DEFINE(idle_work, idle_work_handler);
```

and, at the end of the sample handler:

```c
	k_work_reschedule(&idle_work, K_MSEC(250));
```

*(2 marks: 1 for a delayable work item, 1 for rescheduling it from the sample handler.)*

**Why `k_work_reschedule()`.** It *moves* an already scheduled item to the new time; `k_work_schedule()`
leaves an already scheduled item alone. With `schedule`, the timeout would keep the time set by the
first sample and fire 250 ms after *that*, whatever happened since. Pushing the deadline back on
every sample is exactly what "no sample for 250 ms" means.
*(1 mark)*

**When it fires.** 250 ms is 25 ticks, plus the remainder of the tick in progress: 26 ticks, so
260 ms after the last sample. With the last sample at 500 ms, the message is logged at **760 ms**.
*(1 mark. Accept "at 750 ms, give or take a tick" with the rounding named.)*

### (c) 3 marks

**It hangs, and takes the whole system with it.** On `native_sim` the code runs in zero simulated
time and interrupts arrive only when the CPU is idle, which it never becomes while a thread spins.
The timer's expiry function is never called, `ready` is never set, the clock never advances, and
even `-stop_at` cannot end the run, because that too is a simulated-time event. On a real
microcontroller the same loop would end — the timer interrupt would preempt it — but it would burn
the CPU for the whole wait.
*(2 marks: 1 for the hang, 1 for *why*, i.e. that no time passes and no interrupt can arrive.)*

**The fix** is to wait on something rather than to look at it:

```c
static K_SEM_DEFINE(ready, 0, 1);

static void sample_timer_expired(struct k_timer *timer)
{
	k_sem_give(&ready);
}

/* In the thread: */
	k_sem_take(&ready, K_FOREVER);
```

*(1 mark. Accept `k_poll`, `k_timer_status_sync()`, or a `k_msgq`/zbus wait. A `k_msleep()` inside
the loop is worth half: it lets time pass, so it works, but it is polling.)*

---

## Question 6 - Threads that must agree (13 marks)

### (a) 5 marks

```c
struct sample {
	uint32_t seq;
	int32_t celsius;
};

K_MSGQ_DEFINE(samples, sizeof(struct sample), 4, alignof(struct sample));

/* The sampler, every 100 ms: */
	struct sample s = {.seq = ++seq, .celsius = read_sensor()};

	if (k_msgq_put(&samples, &s, K_NO_WAIT) != 0) {
		dropped++;
	}

/* The storage thread: */
	struct sample s;

	while (k_msgq_get(&samples, &s, K_FOREVER) == 0) {
		store(&s);
	}
```

*(4 marks: 1 for `K_MSGQ_DEFINE` with the message size, the depth and the alignment; 1 for
`K_NO_WAIT` and counting the failure; 1 for `K_FOREVER` in the consumer; 1 for checking both return
values. A producer that passes a timeout, or ignores the result of `k_msgq_put()`, has silently
dropped the sample instead of counting it.)*

**What `k_msgq_put()` does with the struct:** it copies the bytes into the queue's own storage, which
was allocated when the queue was defined. Nothing is shared and nothing is owned, so the producer's
local may go out of scope the moment the call returns — and a reader gets a snapshot, not a
reference to something that may have changed since.
*(1 mark)*

### (b) 4 marks

```c
static K_MUTEX_DEFINE(calibration_lock);

	k_mutex_lock(&calibration_lock, K_FOREVER);
	/* ... the critical section ... */
	k_mutex_unlock(&calibration_lock);
```

*(1 mark)*

**Two things a mutex gives you that a binary semaphore does not** *(2 marks)*:

* **Ownership.** A mutex is held by a thread, and only that thread can unlock it; it is also
  recursive for its owner. A semaphore has no owner: any thread can give it, including one that
  never took it, so an unbalanced give goes unnoticed.
* **Priority inheritance.** While a higher-priority thread waits for the mutex, the kernel raises
  the holder's priority to the waiter's, and restores it at the unlock.

Credit also: a mutex may not be used in an ISR, which is a useful restriction, since an ISR has no
priority to inherit and no identity to own with.

**What the semaphore version suffers from: unbounded priority inversion.** The thread at 7 holds the
guard; the one at 5 blocks on it; the one at 6, which never touches the guard, preempts 7 and runs
for as long as it likes. The highest-priority thread in the system waits for the middle one, and its
wait is bounded by nothing anybody designed.

**With the mutex**, 7 runs at 5's priority while it holds the lock, so 6 cannot preempt it. The
highest-priority thread's wait is bounded by the length of the critical section — which is the thing
the designer controls, and the reason to keep it short.
*(1 mark)*

### (c) 4 marks

```c
static bool sample_is_valid(const void *msg, size_t msg_size)
{
	const struct sample *s = msg;

	ARG_UNUSED(msg_size);
	return (s->celsius >= -40) && (s->celsius <= 125);
}

static void alarm_callback(const struct zbus_channel *chan)
{
	const struct sample *s = zbus_chan_const_msg(chan);

	LOG_INF("alarm saw #%u", s->seq);
}

ZBUS_LISTENER_DEFINE(alarm, alarm_callback);
ZBUS_SUBSCRIBER_DEFINE(display, 4);

ZBUS_CHAN_DEFINE(sample_chan, struct sample, sample_is_valid, NULL,
		 ZBUS_OBSERVERS(alarm, display), ZBUS_MSG_INIT(0));

static void display_thread(void)
{
	const struct zbus_channel *chan;
	struct sample s;

	while (zbus_sub_wait(&display, &chan, K_FOREVER) == 0) {
		zbus_chan_read(chan, &s, K_FOREVER);
		LOG_INF("display saw #%u", s.seq);
	}
}
```

*(3 marks: 1 for the channel with its message type and validator, 1 for the listener, 1 for the
subscriber and its wait-then-read loop. The publisher calls `zbus_chan_pub(&sample_chan, &s, ...)`;
a rejected message gives it `-ENOMSG`.)*

**Where each one runs.** The **listener** is a callback run inside `zbus_chan_pub()`, in the
publisher's own thread and with the channel locked — so it must be quick and must not block, like an
ISR. The **subscriber** runs in its own thread, whenever the scheduler gets to it.

**Why a subscriber can miss a message.** It is not handed the message: it is handed a notification
that the channel changed, and reads the channel later, by which time the channel holds whatever was
published last. Two publications before the subscriber runs leave it reading the second one twice,
or seeing only the newer value. The listener cannot miss one, because it is called during the
publication, before `zbus_chan_pub()` returns.
*(1 mark. Credit the message subscriber, `ZBUS_MSG_SUBSCRIBER_DEFINE`, as the observer that keeps a
copy of every message and so misses none.)*

---

## Question 7 - A driver of your own (13 marks)

### (a) 6 marks

```c
#define DT_DRV_COMPAT qacademy_baro

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "baro_reg.h"

LOG_MODULE_REGISTER(baro, CONFIG_SENSOR_LOG_LEVEL);

struct baro_config {
	struct i2c_dt_spec i2c;
};

struct baro_data {
	uint16_t raw;
};

static int baro_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct baro_config *config = dev->config;
	struct baro_data *data = dev->data;
	uint8_t buf[2];
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_PRESS)) {
		return -ENOTSUP;
	}

	ret = i2c_burst_read_dt(&config->i2c, BARO_REG_PRESSURE_L, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	data->raw = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
	return 0;
}

static int baro_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	const struct baro_data *data = dev->data;

	if (chan != SENSOR_CHAN_PRESS) {
		return -ENOTSUP;
	}

	/* One count is 10 Pa; the sensor API's pressure channel is in kilopascals. */
	val->val1 = data->raw / 100;
	val->val2 = (data->raw % 100) * 10000;
	return 0;
}

static DEVICE_API(sensor, baro_api) = {
	.sample_fetch = baro_sample_fetch,
	.channel_get = baro_channel_get,
};

static int baro_init(const struct device *dev)
{
	const struct baro_config *config = dev->config;
	uint8_t id;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, BARO_REG_WHO_AM_I, &id);
	if (ret < 0) {
		return ret;
	}

	if (id != BARO_WHO_AM_I_VALUE) {
		LOG_ERR("%s: WHO_AM_I is 0x%02x, expected 0x%02x", dev->name, id,
			BARO_WHO_AM_I_VALUE);
		return -ENODEV;
	}

	return 0;
}

#define BARO_DEFINE(inst)                                                                          \
	static struct baro_data baro_data_##inst;                                                  \
	static const struct baro_config baro_config_##inst = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, baro_init, NULL, &baro_data_##inst,                     \
				     &baro_config_##inst, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &baro_api);

DT_INST_FOREACH_STATUS_OKAY(BARO_DEFINE)
```

*(6 marks, one each for:)*

1. **`DT_DRV_COMPAT`** spelled with the comma turned into an underscore, before the includes. Every
   `DT_INST_` macro in the file counts instances of that compatible only.
2. **The config/data split**: what the devicetree says is `const` and lives in `dev->config`; what
   changes at run time lives in `dev->data`. A driver that keeps its `i2c_dt_spec` in `data`, or
   that makes the config non-const, loses this mark — on a board the config is in flash.
3. **`baro_init` checking the bus and the identity**, and returning `-ENODEV` rather than 0 when the
   part answers with the wrong value.
4. **`sample_fetch`** reading both bytes in one burst and storing the raw value, with the channel
   checked.
5. **`channel_get`** converting to `sensor_value` correctly: 10132 counts gives `val1` 101 and
   `val2` 320000, because `val2` is millionths.
6. **`DT_INST_FOREACH_STATUS_OKAY(BARO_DEFINE)`** with `I2C_DT_SPEC_INST_GET`, so that the driver
   serves every node with that compatible rather than one.

*A note for the marker: `SENSOR_DEVICE_DT_INST_DEFINE` and `DEVICE_DT_INST_DEFINE` are both
acceptable, as is a plain `static const struct sensor_driver_api` in place of `DEVICE_API`.*

### (b) 4 marks

```yaml
name: qacademy-baro
build:
  cmake: .
  kconfig: Kconfig
  settings:
    dts_root: .
```

*(1 mark. `dts_root: .` is what makes the module's `dts/bindings` visible to the build; without it
the binding is not found and every node with that compatible has no binding.)*

```text
config BARO
	bool "Baro I2C pressure sensor"
	default y
	depends on SENSOR
	depends on DT_HAS_QACADEMY_BARO_ENABLED
	select I2C
```

*(1 mark)*

```yaml
description: Baro, a made-up I2C pressure sensor reading in steps of 10 Pa.

compatible: "qacademy,baro"

include: [sensor-device.yaml, i2c-device.yaml]
```

*(1 mark. `i2c-device.yaml` is what requires `reg`, the bus address, and makes the node an I2C
device; `sensor-device.yaml` brings the properties every sensor may have.)*

**Why the dependency.** `DT_HAS_QACADEMY_BARO_ENABLED` is generated from the devicetree: it is
defined exactly when some node has that compatible and status okay. Tying the driver to it means the
driver is built when, and only when, the hardware is described — which is why enabling a driver in
Zephyr usually means adding a devicetree node rather than setting an option. An application without
the part gets none of the driver's code, and `default y` costs it nothing.
*(1 mark)*

### (c) 3 marks

```c
#define BARO_NODE DT_NODELABEL(baro)

static const struct device *const baro = DEVICE_DT_GET(BARO_NODE);
static const struct emul *const baro_emul = EMUL_DT_GET(BARO_NODE);

ZTEST_SUITE(baro, NULL, NULL, NULL, NULL, NULL);

ZTEST(baro, test_device_is_ready)
{
	zassert_true(device_is_ready(baro));
}

ZTEST(baro, test_reading_converts_to_kilopascals)
{
	struct sensor_value pressure;

	baro_emul_set_raw(baro_emul, 10132);
	zassert_ok(sensor_sample_fetch(baro));
	zassert_ok(sensor_channel_get(baro, SENSOR_CHAN_PRESS, &pressure));
	zassert_equal(pressure.val1, 101);
	zassert_equal(pressure.val2, 320000);
}
```

*(2 marks: 1 for the suite and the readiness test, 1 for driving the emulator and asserting on both
halves of the `sensor_value`.)*

**The file is `testcase.yaml`**, beside the test application, naming the platform and
`harness: ztest`. `west twister -T <dir> -p native_sim/native/64` then builds and runs it.

**`device_is_ready()` returns false** for a device whose init returned a negative error: the kernel
records the failure and the device stays unusable for the life of the program. The application must
therefore check it before the first call — that is what the check is for, and a driver that fails
its identity check is exactly the case it catches.
*(1 mark)*

---

## Question 8 - Making it usable (12 marks)

### (a) 5 marks

```c
static int32_t setpoint_c = 20;

static int cmd_setpoint_get(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "setpoint %d C", setpoint_c);
	return 0;
}

static int cmd_setpoint_set(const struct shell *sh, size_t argc, char **argv)
{
	char *end;
	long value = strtol(argv[1], &end, 10);

	ARG_UNUSED(argc);

	if ((*end != '\0') || (value < -40) || (value > 125)) {
		shell_error(sh, "setpoint must be -40..125 degrees Celsius, not '%s'", argv[1]);
		return -EINVAL;
	}

	setpoint_c = (int32_t)value;
	shell_print(sh, "setpoint %d C", setpoint_c);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(setpoint_cmds,
			       SHELL_CMD(get, NULL, "Print the setpoint.", cmd_setpoint_get),
			       SHELL_CMD_ARG(set, NULL, "Set the setpoint: set <celsius>.",
					     cmd_setpoint_set, 2, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(setpoint, &setpoint_cmds, "The temperature setpoint.", NULL);
```

*(4 marks: 1 for the handler signature and `shell_print` to the shell that ran the command rather
than `printk`; 1 for the subcommand set and its `SHELL_SUBCMD_SET_END`; 1 for `SHELL_CMD_ARG` with
two mandatory words; 1 for validating the text as well as the range — `strtol` alone accepts
`abc` as 0, which is why the end pointer is checked.)*

**What the shell has already checked:** the argument *count*. `SHELL_CMD_ARG(..., 2, 0)` declares two
mandatory words and no optional ones, so `setpoint set` on its own never reaches the handler; the
shell rejects it with `set: wrong parameter count` and prints the command's help. What is left to
the handler is what the words *say*.

**The return value** is 0 for success or a negative errno; the shell reports a failure to the user,
and it is what a script or a test driving the shell sees.
*(1 mark)*

### (b) 4 marks

```c
static int setpoint_set_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;

	if (settings_name_steq(name, "celsius", &next) && (next == NULL)) {
		if (len != sizeof(setpoint_c)) {
			return -EINVAL;
		}
		ssize_t read = read_cb(cb_arg, &setpoint_c, sizeof(setpoint_c));

		return (read < 0) ? (int)read : 0;
	}
	return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(setpoint, "setpoint", NULL, setpoint_set_cb, NULL, NULL);
```

with, at start-up, `settings_subsys_init()` then `settings_load()`, and in the `set` handler:

```c
	settings_save_one("setpoint/celsius", &setpoint_c, sizeof(setpoint_c));
```

*(2 marks: 1 for a handler registered for the subtree and reading through `read_cb` rather than
expecting a pointer to the value, 1 for `settings_save_one()` with the full name and
`settings_subsys_init()`/`settings_load()` at boot.)*

**The Kconfig options:** `CONFIG_SETTINGS=y`, a storage back-end — `CONFIG_ZMS=y` (or
`CONFIG_NVS=y`) — and the flash underneath it, `CONFIG_FLASH=y` and `CONFIG_FLASH_MAP=y`.
*(1 mark)*

**Where it is kept:** in the flash partition labelled `storage_partition` in the board's devicetree.
On `native_sim` that flash is a file, `flash.bin`, in the directory the program is run from — the
build directory when it is started with `west build -t run`.

**Two ways to make the next run a first boot**, any two of: delete `flash.bin`; start the program
with `-flash_erase`; start it with `-flash_in_ram`, so the flash never touches the disk; or build
with `-p`, which deletes the build directory and the file with it.
*(1 mark)*

### (c) 3 marks

```c
void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	LOG_PANIC();
	LOG_ERR("fatal error %u in thread '%s'", reason, k_thread_name_get(k_current_get()));

	if (reason == K_ERR_KERNEL_OOPS) {
		return;                 /* The kernel aborts this thread only. */
	}

	k_fatal_halt(reason);
}
```

*(2 marks: 1 for replacing the weak handler with one that returns on `K_ERR_KERNEL_OOPS`, 1 for
halting otherwise. `LOG_PANIC()` earns credit as an addition: it puts logging into synchronous mode,
so that what has been logged is actually printed before the system goes down — which matters on a
board, where logging is deferred to a thread that may never run again.)*

**The exit status is 1.** Halting on `native_sim` ends the process, and the harness sees a failure;
that is also why Twister treats such a run as failed whatever it printed.

**Why returning from a panic is not allowed:** an oops means one thread has found something
impossible, and the kernel can abort just that thread and carry on. A panic means the kernel itself
cannot continue — a failed assertion inside it, a corrupted structure, a fault with no thread to
blame — so there is nothing to return *to*. The kernel's own handler halts for exactly that reason.
*(1 mark)*

---
