# Appendix C

## Exercises
These exercises reinforce the concepts from [Appendix A](./a_kernel_objects.md) and
[Appendix B](./b_zbus_and_smf.md).

> **How to check your work.** Write each exercise set as its own application under
> [`lectures/L04/exercises`](../exercises/README.md), with the file names given there, and run
> `make test` from the repository root. A set's tests switch on as soon as its files exist, and a
> set you have not started is reported as `SKIP`. Each application grows through its set, and the
> tests check where it ends up. Programs run on `native_sim` in simulated time, so their output,
> timestamps included, is compared with the expected output line by line. The threads' priorities,
> start delays and sleeps given below are part of the exercise: change one, and the output changes.

Remember `source env.sh` in every new terminal. Build and run an exercise from the repository root:

```bash
west build -p -b native_sim/native/64 lectures/L04/exercises/exercise1
./build/zephyr/zephyr.exe -stop_at=3 -no-rt
```

---

# Exercise Set 1 – Readings in a queue
Create an application named `exercise1` to work in for this exercise set, with a `CMakeLists.txt`,
a `prj.conf` and a `src/main.c`.

## Exercise 1.1 – A sampler and a slower storage thread
In this exercise you will connect a fast producer and a slow consumer with a message queue.

### Tasks
* In `prj.conf`, set one tick per millisecond, as the lecture's examples do:
  `CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000`.
* Define a message type:

  ```c
  struct reading {
  	uint32_t seq;
  	int64_t timestamp_ms;
  };
  ```

* Define a message queue named `readings` with room for **3** readings.
* Write a thread `sampler`, priority **5**, which takes 8 readings, numbered 1 to 8. For each:
  * Fill in a `struct reading` with the number and `k_uptime_get()`.
  * Put it into the queue **without waiting**. If that fails, print
    `[<t> ms] queue full, dropped #<n>`, with the reading's number. Otherwise print
    `[<t> ms] sampled #<n>, queue <used>/3`.
  * Sleep 100 ms.

  After the eighth reading, print `[<t> ms] sampler done`.
* Write a thread `storage`, priority **6**, which forever takes a reading from the queue, waiting as
  long as it takes, prints `[<t> ms]   stored #<n>, <age> ms old` (three spaces after the
  timestamp), where the age is the uptime minus the reading's timestamp, and sleeps 350 ms.
* Start both threads with `K_THREAD_DEFINE()`, a stack of 1024 bytes, and no start delay.

Print every timestamp with `%4lld`. With `-stop_at=3`, the output must be:

```text
[   0 ms] sampled #1, queue 1/3
[   0 ms]   stored #1, 0 ms old
[ 101 ms] sampled #2, queue 1/3
[ 202 ms] sampled #3, queue 2/3
[ 303 ms] sampled #4, queue 3/3
[ 351 ms]   stored #2, 250 ms old
[ 404 ms] sampled #5, queue 3/3
[ 505 ms] queue full, dropped #6
[ 606 ms] queue full, dropped #7
[ 702 ms]   stored #3, 500 ms old
[ 707 ms] sampled #8, queue 3/3
[ 808 ms] sampler done
[1053 ms]   stored #4, 750 ms old
[1404 ms]   stored #5, 1000 ms old
[1755 ms]   stored #8, 1048 ms old
```

**Hint:** `K_MSGQ_DEFINE()`'s last argument is the message's alignment: `alignof(struct reading)`,
from `<stdalign.h>`. `k_msgq_num_used_get()` returns how many messages are queued.

### Reflection
* `sampled #1` is printed before `stored #1`. Why? Give `storage` priority 4 instead, and run it
  again: what changes, and why does the queue read `0/3`?
* Why does the sampler put without waiting, rather than with `K_FOREVER`? Which kind of code could
  not wait at all?

---

## Exercise 1.2 – Keep the newest
### Tasks
Change the sampler so that a full queue drops the **oldest** reading instead of the new one:
* When the put fails, take the oldest reading out of the queue without waiting, print
  `[<t> ms] queue full, dropped #<n>` with **its** number, and put the new reading again.
* Then print `sampled #<n>` for the new reading, as for any other.

The output must now be:

```text
[   0 ms] sampled #1, queue 1/3
[   0 ms]   stored #1, 0 ms old
[ 101 ms] sampled #2, queue 1/3
[ 202 ms] sampled #3, queue 2/3
[ 303 ms] sampled #4, queue 3/3
[ 351 ms]   stored #2, 250 ms old
[ 404 ms] sampled #5, queue 3/3
[ 505 ms] queue full, dropped #3
[ 505 ms] sampled #6, queue 3/3
[ 606 ms] queue full, dropped #4
[ 606 ms] sampled #7, queue 3/3
[ 702 ms]   stored #5, 298 ms old
[ 707 ms] sampled #8, queue 3/3
[ 808 ms] sampler done
[1053 ms]   stored #6, 548 ms old
[1404 ms]   stored #7, 798 ms old
[1755 ms]   stored #8, 1048 ms old
```

### Reflection
* Compare the readings stored, and their ages, with Exercise 1.1. Which policy does a temperature
  logger want? Which would a queue of commands want?
* Make the queue 8 readings long. Is anything dropped now? How old is the last reading when it is
  stored? Did the bigger queue help?
* How much RAM does the queue's buffer use on `native_sim/native/64`, and when is it allocated?
* Dropping the oldest takes two calls, a get and a put. Why is that safe in this program, and when
  would it not be?

---

# Exercise Set 2 – Priority inversion
Create an application named `exercise2` to work in for this exercise set.

## Exercise 2.1 – A log behind a semaphore
In this exercise you will produce priority inversion, measure it, and fix it.

### Tasks
* In `prj.conf`, set `CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000`.
* Create a `Kconfig` file with a menu named `Application` holding a `bool` option
  `APP_USE_MUTEX`, with the prompt `Protect the log with a k_mutex instead of a k_sem`, and end it so
  that Zephyr's own options are still available. The option stays unused until Exercise 2.2.
* In `src/main.c`, protect a shared log with a binary semaphore named `log_lock`, created available
  (initial count 1, limit 1).
* Write a function `trace(who, what)` that prints
  `[<t> ms] <who> (priority <p>) <what>`, with the uptime as `%4lld`, `who` left-aligned in 7
  characters (`%-7s`), and the calling thread's current priority from `k_thread_priority_get()`.
* Write a function `compute(ms)` that uses CPU time by calling `k_busy_wait(USEC_PER_MSEC)` `ms`
  times.
* Write three threads, all with a stack of 1024 bytes:
  * `logger`, priority **9**, no start delay: traces `takes the lock`, takes it, computes for
    **60** ms, traces `gives the lock back`, and gives it back.
  * `control`, priority **5**, start delay **10** ms: records the uptime, traces `needs the lock`,
    takes it, traces `has the lock`, gives it back, and prints `control waited <ms> ms`, the time
    since the uptime it recorded.
  * `network`, priority **7**, start delay **20** ms: traces `starts sending`, computes for **300** ms,
    and traces `is done`.

The output must be:

```text
[   0 ms] logger  (priority 9) takes the lock
[  11 ms] control (priority 5) needs the lock
[  21 ms] network (priority 7) starts sending
[ 321 ms] network (priority 7) is done
[ 360 ms] logger  (priority 9) gives the lock back
[ 360 ms] control (priority 5) has the lock
control waited 349 ms
```

**Hint:** why `compute()` busy-waits one millisecond at a time rather than calling
`k_busy_wait(60000)` once is in Appendix A, *On `native_sim`: making a thread use CPU time*.

### Reflection
* Before you run it, predict how long `control` waits. Then account for the 349 ms, millisecond by
  millisecond: who ran when?
* `network` never touches the lock. How did it delay `control` by 300 ms?

---

## Exercise 2.2 – The mutex
### Tasks
* When `CONFIG_APP_USE_MUTEX` is enabled, protect the log with a `k_mutex` named `log_lock` instead
  of the semaphore, and change nothing else. Hide the difference behind two macros, such as
  `lock()` and `unlock()`, so that the threads' code is the same in both builds.
* Create a file `mutex.conf` that enables the option.

Built with `-DEXTRA_CONF_FILE=mutex.conf`, the output must be:

```text
[   0 ms] logger  (priority 9) takes the lock
[  11 ms] control (priority 5) needs the lock
[  60 ms] logger  (priority 5) gives the lock back
[  60 ms] control (priority 5) has the lock
control waited 49 ms
[  60 ms] network (priority 7) starts sending
[ 360 ms] network (priority 7) is done
```

Built without it, the output must be unchanged from Exercise 2.1.

### Reflection
* Why is `logger`'s priority 5 when it gives the lock back? What is it after the unlock?
* `network` was ready at 21 ms. Why does it only start sending at 60 ms?
* `control` still waited 49 ms. Could any lock bring that down to zero? What would?
* With the mutex, make `control` wait at most 20 ms, with `k_mutex_lock(&log_lock, K_MSEC(20))`,
  and trace `gave up` when that fails. What happens to `logger`'s priority when `control` gives up,
  and what does `network` do then?

---

# Exercise Set 3 – A battery on zbus
Create an application named `exercise3` to work in for this exercise set.

## Exercise 3.1 – The channel and a listener
In this exercise you will publish battery readings on a zbus channel, and react to them.

### Tasks
* Enable zbus in `prj.conf`.
* Define the message type:

  ```c
  struct battery_msg {
  	uint8_t percent;
  	bool charging;
  };
  ```

* Define a channel named `battery_chan`, initially `{.percent = 100, .charging = false}`, with a
  validator that rejects any message with `percent` above 100.
* Define a listener named `led`, observing the channel, whose callback prints `  led: red` (two
  spaces first) when the battery is below 20 % and not charging, and `  led: green` otherwise.
* Write a thread `gauge`, priority **6**, stack 1024, no start delay, that publishes these readings,
  in this order:

  ```c
  static const struct battery_msg readings[] = {
  	{.percent = 80, .charging = false}, {.percent = 15, .charging = false},
  	{.percent = 150, .charging = false}, {.percent = 15, .charging = true},
  	{.percent = 60, .charging = true},
  };
  ```

  For each, it prints `gauge: <percent> %`, followed by `, charging` if the battery is charging,
  publishes the reading with a timeout of 50 ms, prints `gauge: rejected` if the channel rejected
  it, and sleeps 100 ms.

The output must be:

```text
gauge: 80 %
  led: green
gauge: 15 %
  led: red
gauge: 150 %
gauge: rejected
gauge: 15 %, charging
  led: green
gauge: 60 %, charging
  led: green
```

**Hint:** A listener callback reads the message in place with `zbus_chan_const_msg(chan)`. In a
format string, `%%` prints a percent sign.

### Reflection
* In which thread does the `led` callback run? Check it: enable `CONFIG_THREAD_NAME=y`, and print
  `k_thread_name_get(k_current_get())` in the callback.
* What number does `zbus_chan_pub()` return for the 150 % reading on `native_sim`? Why should the
  code compare it with `-ENOMSG` rather than with that number?

---

## Exercise 3.2 – A display that subscribes
### Tasks
* Define a subscriber named `display`, with a notification queue of **4**, and add it to the
  channel's observers after `led`.
* Write a thread `display_thread`, priority **7**, stack 1024, no start delay, which forever waits
  for a notification, reads the channel, and prints `  display: <percent> %`, followed by
  `, charging` if the battery is charging.

The output must now be:

```text
gauge: 80 %
  led: green
  display: 80 %
gauge: 15 %
  led: red
  display: 15 %
gauge: 150 %
gauge: rejected
gauge: 15 %, charging
  led: green
  display: 15 %, charging
gauge: 60 %, charging
  led: green
  display: 60 %, charging
```

### Reflection
* Remove the gauge's 100 ms sleep. What does the display print now, and why? Did it receive four
  notifications?
* Keep the sleep removed, and give `display_thread` priority 5, above the gauge. What does the
  display print now?
* Why does the `led` never miss a reading, whatever the priorities?

---

# Exercise Set 4 – A garage door as a state machine
Create an application named `exercise4` to work in for this exercise set, with
`target_sources(app PRIVATE src/main.c src/door.c)` in its `CMakeLists.txt`.

## Exercise 4.1 – The state machine
In this exercise you will write a hierarchical state machine with SMF, behind an API that tests can
drive directly.

### Tasks
Copy this header into `src/door.h` unchanged. It is the interface the tests use:

```c
#ifndef DOOR_H_
#define DOOR_H_

#include <stdbool.h>

/** What can happen to the door. */
enum door_event {
	DOOR_EVENT_BUTTON,   /**< The button was pressed. */
	DOOR_EVENT_OPENED,   /**< The door reached its top end switch. */
	DOOR_EVENT_CLOSED,   /**< The door reached its bottom end switch. */
	DOOR_EVENT_OBSTACLE, /**< The light barrier saw something in the way. */
};

/** The states the door can be in. */
enum door_state {
	DOOR_STATE_CLOSED,
	DOOR_STATE_OPENING,
	DOOR_STATE_OPEN,
	DOOR_STATE_CLOSING,
	DOOR_STATE_STOPPED,
};

/** @brief Start the state machine, in the closed state, with the motor off
 *         and never started. */
void door_init(void);

/** @brief Run the state machine once, for one event. */
void door_handle(enum door_event event);

/** @brief Return the state the door is in. */
enum door_state door_get_state(void);

/** @brief Return whether the motor is running. */
bool door_motor_on(void);

/** @brief Return how many times the motor has been started since door_init(). */
unsigned int door_motor_starts(void);

#endif /* DOOR_H_ */
```

In `prj.conf`, enable SMF with parent states. In `src/door.c`, implement the header with SMF, with
these six states:

| State | Parent | Entry | Handles |
|-------|--------|-------|---------|
| `MOVING` | none | Motor on, and count the start | `BUTTON`: go to `STOPPED`. Every other event: ignore it. Exit: motor off. |
| `CLOSED` | none | | `BUTTON`: go to `OPENING`. |
| `OPENING` | `MOVING` | | `OPENED`: go to `OPEN`. Anything else: pass it to the parent. |
| `OPEN` | none | | `BUTTON`: go to `CLOSING`. |
| `CLOSING` | `MOVING` | | `CLOSED`: go to `CLOSED`. `OBSTACLE`: go to `OPENING`. Anything else: pass it to the parent. |
| `STOPPED` | none | | `BUTTON`: go to `CLOSING`. |

Events a state does not list are ignored. `door_get_state()` returns the state the door is in, and
`MOVING` is never one: the door is always in one of the five states of the header.

**Hint:** `MOVING`'s entry and exit actions are the only code that touches the motor. Keep the
state in a variable that each leaf state's entry action sets.

---

## Exercise 4.2 – Drive it
### Tasks
In `src/main.c`, call `door_init()`, print the door's status as `start`, and then handle these events
one at a time, printing the status after each, labelled with the event's name:

```c
static const enum door_event script[] = {
	DOOR_EVENT_BUTTON,   DOOR_EVENT_OBSTACLE, DOOR_EVENT_OPENED,
	DOOR_EVENT_BUTTON,   DOOR_EVENT_OBSTACLE, DOOR_EVENT_OPENED,
	DOOR_EVENT_BUTTON,   DOOR_EVENT_BUTTON,   DOOR_EVENT_BUTTON,
	DOOR_EVENT_CLOSED,   DOOR_EVENT_OBSTACLE,
};
```

Print each status with `"%-8s -> %-7s motor %-3s (started %u times)\n"`: the label, the state's name,
`on` or `off`, and the number of motor starts. The output must be:

```text
start    -> CLOSED  motor off (started 0 times)
BUTTON   -> OPENING motor on  (started 1 times)
OBSTACLE -> OPENING motor on  (started 1 times)
OPENED   -> OPEN    motor off (started 1 times)
BUTTON   -> CLOSING motor on  (started 2 times)
OBSTACLE -> OPENING motor on  (started 2 times)
OPENED   -> OPEN    motor off (started 2 times)
BUTTON   -> CLOSING motor on  (started 3 times)
BUTTON   -> STOPPED motor off (started 3 times)
BUTTON   -> CLOSING motor on  (started 4 times)
CLOSED   -> CLOSED  motor off (started 4 times)
OBSTACLE -> CLOSED  motor off (started 4 times)
```

The tests also drive `door.c` directly, without `main.c`, through sequences of events the script
does not contain.

### Reflection
* `OBSTACLE` while closing sends the door back to `OPENING`, and the motor start count stays at 2.
  Which rule of SMF makes that happen?
* Which state handles `OBSTACLE` while the door is opening, and what does it do with it?
* Change `OPENING`'s run action to return `SMF_EVENT_HANDLED` for every event. What happens when the
  button is pressed while the door is opening? Which tests fail?
* Remove `CONFIG_SMF_ANCESTOR_SUPPORT=y` from `prj.conf`. Does it still build? What does the program
  print now, and why?
* Why can `door.c` be tested without threads, timers, or `native_sim`'s clock?

---
