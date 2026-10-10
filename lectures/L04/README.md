# L04 - Kernel II: Communication and Application Architecture

## Agenda
* Kernel objects for synchronization: semaphores, mutexes and events.
* Priority inversion: how it happens, and how a mutex's priority inheritance bounds it.
* Kernel objects for passing data: message queues, FIFOs, and `k_poll()` for waiting on several.
* Blocking, timeouts and `K_NO_WAIT`: what a thread should do when a queue is full.
* Memory without `malloc()`: static allocation, `k_heap` and memory slabs.
* zbus: channels, validators, listeners, subscribers and message subscribers.
* The State Machine Framework: states, transitions, entry, run and exit actions, and parent states.
* Structuring an application so that its parts know about channels and events, not about each
  other.

---

## Objectives
After this lecture, participants should:
* Be able to choose the right kernel object for a synchronization or communication problem.
* Understand why a semaphore used as a lock allows unbounded priority inversion, and a mutex does
  not.
* Be able to pass data between threads with a message queue, and handle a full queue deliberately.
* Understand how Zephyr applications avoid dynamic memory, and what to use when they cannot.
* Be able to decouple an application's parts with zbus channels, and choose between its kinds of
  observer.
* Be able to express a device's behaviour as a hierarchical state machine with SMF.

---

## Prerequisites
* L03: threads, priorities, time and work queues.

---

## Instructions

### Preparation
* Read [Appendix A](./appendix/a_kernel_objects.md) and [Appendix B](./appendix/b_zbus_and_smf.md).
* For further reading, the Zephyr documentation's
  [Mutexes](https://docs.zephyrproject.org/4.4.0/kernel/services/synchronization/mutexes.html),
  [Message Queues](https://docs.zephyrproject.org/4.4.0/kernel/services/data_passing/message_queues.html),
  [Zephyr bus (zbus)](https://docs.zephyrproject.org/4.4.0/services/zbus/index.html) and
  [State Machine Framework](https://docs.zephyrproject.org/4.4.0/services/smf/index.html).

### During the Lecture
* Participate in the lecture (live coding session). The examples are in
  [`examples/`](./examples):
    * [`msgq`](./examples/msgq/README.md): a producer and a slower consumer, a queue between them,
      and what the producer does when it is full.
    * [`priority_inversion`](./examples/priority_inversion/README.md): three threads and one lock,
      as a semaphore and as a mutex, with the high-priority thread's wait measured.
    * [`zbus`](./examples/zbus/README.md): one channel, a validator, and three kinds of observer.
    * [`smf`](./examples/smf/README.md): a heater controller as a hierarchical state machine, driven
      by events.

### After the Lecture
* Complete the exercises in [Appendix C](./appendix/c_exercises.md).
* Write them in [`exercises/`](./exercises/README.md) and check them with `make test` from the
  repository root.
* Exercise solutions are published in `appendix/solutions` a few days after the lecture.

---

## Evaluation
Participants should be able to explain:
* What is the difference between a semaphore and a mutex, beyond the count?
* What is priority inversion, and why is the medium-priority thread the one that makes it
  unbounded?
* How does priority inheritance bound it, and what does it not solve?
* When should a producer use `K_NO_WAIT` on a full queue, and when a timeout?
* Why do Zephyr applications usually allocate kernel objects and buffers statically?
* What is the difference between a zbus listener, a subscriber and a message subscriber, and in
  which thread does each one run?
* Why can a zbus subscriber miss a message, and which observer cannot?
* What does a parent state in SMF save you from writing?
* On `native_sim`, why must a thread that simulates CPU work use `k_busy_wait()` rather than a loop?

---

## Next Lecture
Writing a driver:
* The device driver model: API structs, config and data, instances and init levels.
* A binding and a Kconfig option for a new part.
* Emulators: hardware that exists only in software, on `native_sim`'s I2C bus.
* Testing the driver with ztest, and running the tests with Twister.

Participants will write a driver for an I2C temperature sensor that does not exist.

---
