# L03 - Kernel I: Threads, Time and Work Queues

## Agenda
* Threads in Zephyr: `K_THREAD_DEFINE()`, `k_thread_create()`, stacks, names and states.
* Priorities: cooperative (negative) and preemptible (non-negative) threads, and the `main` thread.
* When the scheduler runs: blocking, `k_yield()`, and a higher-priority thread becoming ready.
* Time: ticks, timeouts, `k_sleep()`, `k_uptime_get()`, and why a sleep ends on a tick.
* Kernel timers, and expiry functions that run in interrupt context.
* Work queues: the system work queue, delayable work, and work queues of your own.
* Choosing between an ISR, a work item and a thread.
* `native_sim`'s time model: why a busy loop stops the world, and what that says about real
  hardware.
* Stack sizing without a stack analyzer: `CONFIG_STACK_USAGE` and `-fstack-usage`.

---

## Objectives
After this lecture, participants should:
* Be able to create threads statically and dynamically, and choose their priorities.
* Understand exactly when a cooperative and a preemptible thread can lose the CPU.
* Be able to predict the order in which a set of threads runs.
* Understand how timeouts are converted to ticks, and what that does to the length of a sleep.
* Understand which context a timer's expiry function runs in, and what may not be done there.
* Be able to defer work from interrupt context to a work queue, and reschedule delayable work.
* Understand what `native_sim` simulates faithfully about scheduling, and what it does not.
* Be able to estimate a thread's stack needs from static analysis.

---

## Prerequisites
* L01 and L02.
* Basic RTOS concepts: threads, priorities, blocking and interrupts. The
  **Real-Time Kernel Design** course covers what is underneath them.

---

## Instructions

### Preparation
* Read [Appendix A](./appendix/a_threads_and_time.md).
* For more depth, the Zephyr documentation's
  [Threads](https://docs.zephyrproject.org/4.4.0/kernel/services/threads/index.html),
  [Scheduling](https://docs.zephyrproject.org/4.4.0/kernel/services/scheduling/index.html), and
  *Important limitations* in
  [the native simulator's architecture](https://docs.zephyrproject.org/4.4.0/boards/native/doc/arch_soc.html).

### During the Lecture
* Participate in the lecture (live coding session). The examples are in
  [`examples/`](./examples):
    * [`threads`](./examples/threads/README.md): cooperative threads, preemption and `k_yield()`,
      as a trace of who runs when.
    * [`timer_work`](./examples/timer_work/README.md): a timer in interrupt context, the system work
      queue, delayable work, and a work queue of its own.
    * [`busy_loop`](./examples/busy_loop/README.md): a thread waiting for a timer, correctly and with
      a busy loop that stalls `native_sim`.
    * [`stack_usage`](./examples/stack_usage/README.md): per-function stack usage from the compiler,
      since `native_sim` has no stack analyzer.

### After the Lecture
* Complete the exercises in [Appendix B](./appendix/b_exercises.md).
* Write them in [`exercises/`](./exercises/README.md) and check them with `make test` from the
  repository root.
* Exercise solutions are published in `appendix/solutions` a few days after the lecture.
* For more depth, the Zephyr documentation's
  [Workqueue threads](https://docs.zephyrproject.org/4.4.0/kernel/services/threads/workqueue.html)
  and [Timers](https://docs.zephyrproject.org/4.4.0/kernel/services/timing/timers.html).

---

## Evaluation
Participants should be able to explain:
* When can a cooperative thread lose the CPU, and when can a preemptible one?
* Why does `k_yield()` sometimes return at once, even though other threads are ready?
* Why does `k_msleep(30)` return after 40 ms on a 100 Hz tick, while a 500 ms one-shot timer fires
  at exactly 500 ms?
* What must not be done in a timer's expiry function, and why?
* Why is the system work queue a cooperative thread, and what does that mean for the work items on
  it?
* When would you use a work queue of your own rather than the system work queue?
* Why does `while (!flag) {}` stall `native_sim` forever, when on a microcontroller it would only
  waste power?
* Why are Zephyr's thread analyzer and stack overflow checks not available on `native_sim`?

---

## Next Lecture
Kernel II, communication and application architecture:
* Semaphores, mutexes, message queues and events.
* Priority inversion, produced and then fixed.
* zbus: publish and subscribe between parts of an application.
* The State Machine Framework.

Participants will produce priority inversion on purpose, and measure what a mutex does about it.

---
