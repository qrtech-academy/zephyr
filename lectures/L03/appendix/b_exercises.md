# Appendix B

## Exercises
These exercises reinforce the concepts from [Appendix A](./a_threads_and_time.md).

> **How to check your work.** Write each exercise set as its own application under
> [`lectures/L03/exercises`](../exercises/README.md), with the file names given there, and run
> `make test` from the repository root. A set's tests switch on as soon as its files exist, and a
> set you have not started is reported as `SKIP`. Each application grows through its set, and the
> tests check where it ends up. Programs run on `native_sim` in simulated time, so their output,
> timestamps included, is compared with the expected output line by line.

Remember `source env.sh` in every new terminal. Build and run an exercise from the repository root:

```bash
west build -p -b native_sim/native/64 lectures/L03/exercises/exercise1
./build/zephyr/zephyr.exe -stop_at=2 -no-rt
```

A program that never stops by itself, even with `-stop_at`, is stuck in a busy loop: press `Ctrl+C`
**twice**.

---

# Exercise Set 1 – Who runs when
Create an application named `exercise1` to work in for this exercise set, with a `CMakeLists.txt`
as in L01, and `CONFIG_THREAD_NAME=y` in `prj.conf`.

Every line the program prints comes from this function, which prints the uptime, and the name and
priority of the thread that calls it. Copy it into `src/main.c`:

```c
static void trace(const char *what)
{
	k_tid_t self = k_current_get();

	printk("%4lld ms  %-7s (%2d)  %s\n", k_uptime_get(), k_thread_name_get(self),
	       k_thread_priority_get(self), what);
}
```

## Exercise 1.1 – Three threads and main
### Tasks
* Write an entry function `periodic()` for a thread, which for now only calls `trace("started")`.
* Define three threads with `K_THREAD_DEFINE()`, all running `periodic()`, each with a stack of
  1024 bytes, no options and no start delay:

  | Thread | Priority |
  |--------|----------|
  | `control` | -1 |
  | `sensor` | 3 |
  | `logger` | 7 |

* In `main()`, call `trace("started")` and return.

Before you run it, write down the order in which you expect the four lines. Then run it.

### Reflection
* In which order are the four lines printed? Why does `control` print before `main`, while `sensor`
  and `logger` print after it?
* Change `control`'s priority to 1 and run it again. What changes, and why?

---

## Exercise 1.2 – Periodic threads
### Tasks
Pass each thread two numbers through its first two arguments, with `INT_TO_POINTER()`, and read
them in `periodic()` with `POINTER_TO_INT()`:

| Thread | `p1`: period, in ms | `p2`: work, in ms |
|--------|---------------------|-------------------|
| `control` | 300 | 150 |
| `sensor` | 200 | 0 |
| `logger` | 100 | 0 |

Extend `periodic()` so that, after `trace("started")`, it runs one round per period up to 600 ms:

```c
for (int round = 1; round * period_ms <= 600; round++) {
	k_msleep(period_ms);
	trace("awake");
	if (work_ms > 0) {
		k_busy_wait(work_ms * USEC_PER_MSEC);
		trace("work done");
	}
}
```

`k_busy_wait()` stands for work that keeps the CPU busy: unlike a loop, it lets simulated time pass.

### Reflection
* At which uptimes does `logger` wake up? Why are they not multiples of 100?
* `logger`'s third round should start at 330 ms. When is it printed, and why?

---

## Exercise 1.3 – On the dot
### Tasks
Replace the relative sleep with an **absolute** one, so that each thread wakes at exactly every
multiple of its period, up to and including 600 ms. The loop becomes a loop over the next wake-up
time, `next`, from `period_ms` to 600 in steps of `period_ms`, with `k_sleep(K_TIMEOUT_ABS_MS(next))`
at the top of each round.

The output must be:

```text
   0 ms  control (-1)  started
   0 ms  main    ( 0)  started
   0 ms  sensor  ( 3)  started
   0 ms  logger  ( 7)  started
 100 ms  logger  ( 7)  awake
 200 ms  sensor  ( 3)  awake
 200 ms  logger  ( 7)  awake
 300 ms  control (-1)  awake
 450 ms  control (-1)  work done
 450 ms  sensor  ( 3)  awake
 450 ms  logger  ( 7)  awake
 460 ms  logger  ( 7)  awake
 500 ms  logger  ( 7)  awake
 600 ms  control (-1)  awake
 750 ms  control (-1)  work done
 750 ms  sensor  ( 3)  awake
 750 ms  logger  ( 7)  awake
```

**Hint:** `next` must be an `int64_t`, like the uptime.

### Reflection
* Which of `logger`'s rounds is printed at 450 ms, which at 460 ms, and why is the second one 10 ms
  later rather than at the same moment?
* Which round of `sensor` is printed at 450 ms?
* `control` is cooperative. Make it preemptible, with priority 1. Does anything change between 300
  and 450 ms? Why, or why not?
* Keep `control` at priority 1 and give `sensor` priority 0. What happens at 400 ms now? Why is
  `control`'s work still done at 450 ms, although it was interrupted?
* Replace `k_busy_wait()` with a `for` loop that counts to a few hundred million. What happens to
  the trace on `native_sim`, and why would it be different on a microcontroller?

---

# Exercise Set 2 – A timer and the system work queue
Create an application named `exercise2` to work in for this exercise set, with
`CONFIG_THREAD_NAME=y` in `prj.conf`.

Use this function to name the context that code runs in:

```c
static const char *context(void)
{
	return k_is_in_isr() ? "ISR" : k_thread_name_get(k_current_get());
}
```

## Exercise 2.1 – From ISR to thread
### Tasks
* Define a work item `sample_work` with `K_WORK_DEFINE()`, whose handler `sample_handler()` counts
  the samples it has taken.
* Define a timer `sample_timer` with `K_TIMER_DEFINE()`, whose expiry function `timer_expired()`
  stores `context()` in a variable `expiry_context` and submits `sample_work` to the system work
  queue.
* In `main()`, start the timer with a delay of 250 ms and a period of 250 ms, then print
  `timer started in <context>`, prefixed with the uptime as in the output below, and return.
* In `sample_handler()`, print a line for each sample with its number, `expiry_context` and
  `context()`, and stop the timer with `k_timer_stop()` after the fourth sample.

Every line starts with the uptime, formatted with `"%4lld ms  "`. Exercise 2.2 gives the lines their
final form.

---

## Exercise 2.2 – What a submission does, and a stop function
### Tasks
* In `timer_expired()`, submit `sample_work` **twice**, and store the two return values of
  `k_work_submit()`.
* Give the timer a stop function, `timer_stopped()`, which prints
  `timer stopped, stop function in <context>`.
* Print the return values and both contexts in `sample_handler()`.

The output must be:

```text
   0 ms  timer started in main
 250 ms  sample 1: expiry in ISR, submitted 1 then 0, handler in sysworkq
 500 ms  sample 2: expiry in ISR, submitted 1 then 0, handler in sysworkq
 750 ms  sample 3: expiry in ISR, submitted 1 then 0, handler in sysworkq
1000 ms  sample 4: expiry in ISR, submitted 1 then 0, handler in sysworkq
1000 ms  timer stopped, stop function in sysworkq
```

### Reflection
* The work was submitted twice for every expiry. How many times did the handler run, and what do
  the return values 1 and 0 mean?
* Why does the stop function run in `sysworkq`, and not in an ISR like the expiry function?
* The samples come at 250, 500, 750 and 1000 ms, while `k_msleep(250)` in a loop would wake at 260,
  520, and so on. Why do the timer's expiries not drift?
* Add `k_msleep(10)` to `timer_expired()` and run it. Then build again with
  `-- -DCONFIG_ASSERT=y` and run it. What happens each time, and what does that tell you about
  assertions?
* Remove the `k_msleep(10)`, and instead add `k_msleep(300)` to `sample_handler()`, after the
  print and before the check that stops the timer, so that handling a sample takes longer than the
  period. When are the samples taken now? What does
  `k_work_submit()` return, and why is there a fifth sample, after the timer was stopped?

---

# Exercise Set 3 – Debouncing
Create an application named `exercise3` to work in for this exercise set.

A mechanical button does not change level once when pressed: its contacts bounce, and the pin
toggles several times over a few milliseconds. **Debouncing** means reporting a new level only when
the pin has stayed at it for a while. This set uses L02's emulated GPIO controller, and a thread
that plays a finger with bouncing contacts.

Use these files as they are. `prj.conf`:

```text
# The GPIO API, for the button.
CONFIG_GPIO=y

# A tick every millisecond, so that a bounce a few milliseconds long can be timed.
CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000
```

`app.overlay`, a button on pin 1 of the emulated GPIO controller:

```dts
#include <zephyr/dt-bindings/gpio/gpio.h>

/ {
	aliases {
		sw0 = &button0;
	};

	buttons {
		compatible = "gpio-keys";

		button0: button_0 {
			gpios = <&gpio0 1 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
			label = "Button";
		};
	};
};
```

And in `src/main.c`, the finger: a press that bounces, a release that bounces, and a glitch too
short to be a press. `edges` is the count of edges your callback has seen, below.

```c
/* Set the button's physical level: 0 is pressed, since the button is active low. */
static void level(int value, int hold_ms)
{
	gpio_emul_input_set(button.port, button.pin, value);
	k_msleep(hold_ms);
}

static void finger(void *p1, void *p2, void *p3)
{
	k_sleep(K_TIMEOUT_ABS_MS(100));
	level(0, 2);
	level(1, 1);
	level(0, 3);
	level(1, 1);
	level(0, 300);

	level(1, 4);
	level(0, 2);
	level(1, 300);

	level(0, 5);
	level(1, 300);

	printk("%4lld ms  finger done, %ld edges\n", k_uptime_get(), (long)atomic_get(&edges));
}

K_THREAD_DEFINE(finger_thread, 1024, finger, NULL, NULL, NULL, 7, 0, 0);
```

## Exercise 3.1 – Every edge
### Tasks
* Get the button as a `struct gpio_dt_spec` named `button`, from the `sw0` alias.
* In `main()`, check that the button is ready, configure it as an input, configure an interrupt on
  **both** edges, and add a callback.
* In the callback, count every edge in an `atomic_t` named `edges`.

Run it: the finger reports 10 edges at 1028 ms.

---

## Exercise 3.2 – Only real presses
### Tasks
* Define a delayable work item, `debounce_work`, with a handler that reads the button's logical
  level with `gpio_pin_get_dt()`, and prints a line only when that level differs from the last one
  it reported (the button starts released):

  ```text
  <uptime> ms  pressed, after <edges> edges
  <uptime> ms  released, after <edges> edges
  ```

* In the callback, after counting the edge, (re)schedule `debounce_work` to run 20 ms after the
  **last** edge.

The output must be:

```text
 132 ms  pressed, after 5 edges
 441 ms  released, after 8 edges
1028 ms  finger done, 10 edges
```

**Hint:** Of `k_work_schedule()` and `k_work_reschedule()`, only one moves an already scheduled
deadline.

### Reflection
* The last edge of the press is at 111 ms. Why is the press reported at 132 ms, and not at 131?
* Why is the glitch at 721 ms, two edges, never reported?
* Try the other one of `k_work_schedule()` and `k_work_reschedule()`. When are the press and the
  release reported now? The output still has the same events: describe a finger for which it would
  not.
* On `native_sim`, the callback runs in the finger's thread. In which context would it run on a
  microcontroller, and why is it still correct to call your scheduling function from it?
* Only the work handler reads and writes the variable holding the last reported level, so it needs
  no lock. Why can the handler not run twice at the same time?
* Set `CONFIG_SYS_CLOCK_TICKS_PER_SEC=100` and run it. When are the press and the release reported,
  and why?

---

# Exercise Set 4 – Stop spinning
Create an application named `exercise4` to work in for this exercise set, with a `prj.conf` holding
only a comment.

This program reads three channels of a converter, whose conversion takes 40 ms, simulated with a
timer. Copy it into `src/main.c`:

```c
#include <stdbool.h>

#include <zephyr/kernel.h>

/* How long the simulated conversion takes. */
#define CONVERSION_MS 40

/* Set by the timer's expiry function when the conversion is done. */
static volatile bool conversion_done;

static void conversion_expired(struct k_timer *timer)
{
	conversion_done = true;
}

K_TIMER_DEFINE(conversion_timer, conversion_expired, NULL);

/* Start a conversion on a channel, wait for it to finish, and return the result. */
static int convert(int channel)
{
	conversion_done = false;
	k_timer_start(&conversion_timer, K_MSEC(CONVERSION_MS), K_NO_WAIT);

	while (!conversion_done) {
	}

	return 1000 + channel * 111;
}

int main(void)
{
	for (int channel = 0; channel < 3; channel++) {
		const int value = convert(channel);

		printk("%3lld ms  channel %d: %d\n", k_uptime_get(), channel, value);
	}
	printk("%3lld ms  all channels converted\n", k_uptime_get());

	return 0;
}
```

## Exercise 4.1 – The stall
### Tasks
Build it, and run it with `./build/zephyr/zephyr.exe -stop_at=1 -no-rt`. Then run `make test`.

### Reflection
* Why does the program print nothing, not even channel 0, and why does `-stop_at=1` not stop it?
* What happens when you press `Ctrl+C` once? And twice?
* What does `make test` report for the set, and after how long?
* Would this program work on a microcontroller? Would it be a good program there?
* Would `k_yield()` inside the loop fix it on `native_sim`? Try it, and explain.

---

## Exercise 4.2 – A semaphore
### Tasks
Replace the flag with a semaphore: the expiry function gives it, and `convert()` takes it with
`K_FOREVER` instead of spinning.

The output must be:

```text
 40 ms  channel 0: 1000
 80 ms  channel 1: 1111
120 ms  channel 2: 1222
120 ms  all channels converted
```

---

## Exercise 4.3 – No semaphore at all
### Tasks
Remove the semaphore and the expiry function as well, define the timer with
`K_TIMER_DEFINE(conversion_timer, NULL, NULL)`, and wait for it in `convert()` with
`k_timer_status_sync()`. The output must be unchanged.

### Reflection
* Why do the conversions finish at exactly 40, 80 and 120 ms?
* Suppose the timer expired before `convert()` started to wait. Would the semaphore version still
  work? Would the `k_timer_status_sync()` version?
* Zephyr's documentation of `native_sim` suggests calling `k_cpu_idle()` inside a busy loop that
  cannot be removed. Try it in the original program. Does it work, and why is 4.3 still the better
  fix?

---

# Exercise Set 5 – Stacks without a stack analyzer
No code in this set: answer the questions by building and looking.

## Exercise 5.1 – Frames
### Tasks
Build your `exercise2` with static stack usage analysis:

```bash
west build -p -b native_sim/native/64 lectures/L03/exercises/exercise2 -- -DCONFIG_STACK_USAGE=y
```

Answer, using `build/CMakeFiles/app.dir/src/main.c.su` and the other `.su` files in `build/`:
* How large is `sample_handler()`'s stack frame?
* On which thread's stack does `sample_handler()` run, and how large is that stack? Find the
  option that sets it in `build/zephyr/.config`.
* Is `sample_handler()`'s frame all it needs? Which functions does it call, and how large are their
  frames?
* On a microcontroller, on which stack would `timer_expired()` run, and which option sets its size?

---

## Exercise 5.2 – What does not work here
### Tasks
Build `exercise2` again with `-- -DCONFIG_THREAD_ANALYZER=y`.

### Reflection
* What happens, and which dependency is not met?
* Why can a thread analyzer that measures how much of each thread's stack was used not work on
  `native_sim`?

---
