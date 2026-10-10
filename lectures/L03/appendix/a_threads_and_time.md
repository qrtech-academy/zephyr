# Appendix A
## Threads, Time and Work Queues
This appendix covers the part of the kernel that decides what runs when: threads and their
priorities, the scheduler's rules, time and timeouts, kernel timers, interrupt context, and work
queues. It ends with the two places where `native_sim` behaves differently from a microcontroller:
code that spins, and stacks. By the end you should be able to predict, line by line, the traces of
the [`threads`](../examples/threads) and [`timer_work`](../examples/timer_work) examples, and
explain why [`busy_loop`](../examples/busy_loop) never finishes.

If you have taken **Real-Time Kernel Design**, you have built everything below yourself. Zephyr's
version has more features and different names, but the same shape.

---

## Threads
### What a thread is
A Zephyr thread is:
* an **entry function**, `void entry(void *p1, void *p2, void *p3)`, with three arguments of the
  caller's choosing;
* a **stack** of its own, whose size is fixed when the thread is defined;
* a **`struct k_thread`**, the kernel's bookkeeping: saved registers, state, priority, name;
* a **priority**, and a few **options**.

Returning from the entry function ends the thread. There is no process around it and no memory
protection between threads on most targets: every thread sees the same global variables.

### Creating a thread at build time
Most threads in an embedded application exist for the whole of its life, and are best defined
statically:

```c
K_THREAD_DEFINE(sensor, 1024, sensor_entry, NULL, NULL, NULL, 3, 0, 0);
```

The arguments are the thread's name, its stack size in bytes, the entry function and its three
arguments, the priority, the options, and a **start delay** in milliseconds. `K_THREAD_DEFINE()`
allocates the stack and the `struct k_thread` at compile time; nothing is allocated at run time.
The name becomes both a C identifier, `sensor`, of type `k_tid_t`, and the thread's name for
`k_thread_name_get()` when `CONFIG_THREAD_NAME=y`.

Statically defined threads are started by the kernel **after every `APPLICATION`-level
initialization and immediately before `main()` is called**, in the `main` thread
(`z_init_static_threads()` in `deps/zephyr/kernel/init.c`). It starts them with the scheduler
locked, so that none of them runs until all of them have been started, and then unlocks it. From
that moment, the scheduler's ordinary rules decide who runs first, `main` included.

A delay of `K_FOREVER` instead of a number creates the thread without starting it;
`k_thread_start()` starts it later.

### Creating a thread at run time
A thread whose existence depends on run-time conditions is created with `k_thread_create()`. Its
stack must still be allocated, usually statically:

```c
K_THREAD_STACK_DEFINE(worker_stack, 1024);
static struct k_thread worker_thread;

k_tid_t tid = k_thread_create(&worker_thread, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack),
                              worker_entry, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
k_thread_name_set(tid, "worker");
```

Use `K_THREAD_STACK_DEFINE()` and `K_THREAD_STACK_SIZEOF()` rather than a `char` array and
`sizeof`: on some targets a thread stack needs guard areas and alignment that the macros add.
`k_thread_join()` waits for a thread to end, and `k_thread_abort()` ends one from outside.

### Thread states
At any moment a thread is in one of these states (the flags are in
`deps/zephyr/include/zephyr/kernel_structs.h`):

| State | Meaning |
|-------|---------|
| **Ready** | Could run, and is waiting for the scheduler to pick it. |
| **Running** | Has the CPU. One thread per CPU. |
| **Pending** | Waiting for a kernel object: a semaphore, a queue, a mutex, a timer's status. |
| **Sleeping** | Waiting for time to pass, in `k_sleep()` and its relatives. |
| **Suspended** | Stopped by `k_thread_suspend()` until `k_thread_resume()`. |
| **Dead** | Returned from its entry function, or aborted. |

Pending and sleeping threads are together called **blocked**. The scheduler only ever chooses among
ready threads.

---

## Priorities
Every thread has an integer priority, and **a lower number is a higher priority**. The range is
split in two:

| Priorities | Kind | Defaults |
|------------|------|----------|
| -16 to -1 | **Cooperative** | `CONFIG_NUM_COOP_PRIORITIES=16`. `K_PRIO_COOP(0)` is -16, `K_PRIO_COOP(15)` is -1. |
| 0 to 14 | **Preemptible** | `CONFIG_NUM_PREEMPT_PRIORITIES=15`. `K_PRIO_PREEMPT(x)` is `x`. |
| 15 | The idle thread | `K_IDLE_PRIO`, lower than any application thread. |

The difference between the two kinds is the most important thing in this lecture:
* A **cooperative** thread, once running, keeps the CPU until it gives it up itself: by blocking,
  sleeping, yielding or returning. Not even a cooperative thread of higher priority can take it.
* A **preemptible** thread loses the CPU the moment a thread of **higher** priority becomes ready.

Interrupts are not threads, and interrupt every thread, cooperative or not. What a cooperative
thread is protected from is other *threads* running before it gives up the CPU, even if an
interrupt made them ready.

A few threads have priorities worth knowing:

| Thread | Priority | Set by |
|--------|----------|--------|
| `main` | 0 | `CONFIG_MAIN_THREAD_PRIORITY` |
| The system work queue, `sysworkq` | -1 | `CONFIG_SYSTEM_WORKQUEUE_PRIORITY` |
| Idle | 15 | `K_IDLE_PRIO` |

`k_thread_priority_set()` changes a thread's priority at run time.

**Meta-IRQ priorities** are the highest cooperative priorities, reserved with
`CONFIG_NUM_METAIRQ_PRIORITIES` (0 by default, so there are none). A ready meta-IRQ thread runs even
before a running cooperative thread, and even when the scheduler is locked. They exist for drivers
that need a thread which behaves almost like an interrupt handler, and applications should not
need them.

---

## When the scheduler runs
The rule, with the exception that makes it interesting: **the running thread is the
highest-priority ready thread, unless the running thread is cooperative**. The scheduler applies it
at a small number of moments:

| Moment | What can happen |
|--------|-----------------|
| The running thread **blocks, sleeps or returns** | The highest-priority ready thread runs, whatever the kind of the thread that stopped. |
| The running thread **makes another thread ready**, e.g. with `k_sem_give()` | If the running thread is preemptible and the other thread has a higher priority, the switch happens *inside* `k_sem_give()`, before it returns. |
| An **interrupt makes a thread ready**, e.g. a timer expires | Same rule, when the interrupt handler returns. |
| The running thread calls **`k_yield()`** | Another ready thread of the **same or higher** priority runs, if there is one. If there is none, `k_yield()` returns at once. |
| A **time slice** ends | With `CONFIG_TIMESLICING=y` (the default), a preemptible thread that has run for `CONFIG_TIMESLICE_SIZE` ms (20 by default) gives way to a ready thread of the same priority. Priorities above `CONFIG_TIMESLICE_PRIORITY` (0) are not sliced. |

Two rules about equal priorities follow from the table and surprise people. A thread made ready
does **not** preempt a running thread of its own priority: it waits until the running thread
blocks, yields or runs out of its time slice. And `k_yield()` never gives the CPU to a lower
priority, which is why it cannot be used to let a lower-priority thread run.

**`k_sched_lock()`** makes the running thread behave as if it were cooperative until
`k_sched_unlock()`. It is how the kernel starts the static threads, and a cheap way to keep a short
sequence from being interleaved with other threads. Interrupts still run.

The [`threads`](../examples/threads) example shows each of these moments in turn, as a trace that
is the same on every run.

---

## Time
### Ticks
The kernel counts time in **ticks** of its system clock, and every timeout is kept as a number of
ticks. `CONFIG_SYS_CLOCK_TICKS_PER_SEC` sets the rate, and its defaults, in
`deps/zephyr/kernel/Kconfig`, are 100 for `native_sim` and QEMU, which makes a tick 10 ms, and
10000 for a tickless kernel (`CONFIG_TICKLESS_KERNEL`), as on many boards. `native_sim` is
tickless too, but of the two defaults the first one that applies wins, so its tick stays 10 ms.
**Nothing about time is finer than a tick**: a 25 ms timeout on `native_sim` becomes three ticks.

`k_uptime_get()` returns the milliseconds since boot, and `k_uptime_ticks()` the ticks.

### Timeouts
Every call that can wait takes a `k_timeout_t`:

| Timeout | Meaning |
|---------|---------|
| `K_NO_WAIT` | Do not wait: succeed at once or fail at once. |
| `K_FOREVER` | Wait for as long as it takes. |
| `K_MSEC(ms)`, `K_SECONDS(s)`, `K_USEC(us)`, `K_TICKS(t)` | Wait at most this long, **relative** to now. |
| `K_TIMEOUT_ABS_MS(ms)`, `K_TIMEOUT_ABS_TICKS(t)` | Wait until the uptime reaches this value. Needs `CONFIG_TIMEOUT_64BIT`, which is on by default. |

`k_msleep(ms)` is `k_sleep(K_MSEC(ms))`.

### Why a relative timeout lasts one tick longer
A thread that asks to sleep for three ticks does so at some moment *during* the current tick, and
the kernel does not know how much of that tick has already passed. To guarantee **at least** the
time asked for, it adds one tick. The line in `z_add_timeout()`, in
`deps/zephyr/kernel/timeout.c`, is:

```c
to->dticks = timeout.ticks + 1 + ticks_elapsed;
```

So on `native_sim`, `k_msleep(30)` returns after 40 ms, and `k_msleep(500)` after 510. The
duration is first rounded **up** to whole ticks, so `k_msleep(25)` also returns after 40 ms.

A loop that sleeps for its period after doing its work therefore **drifts**: each round starts a
tick later than the one before, plus however long the work took. **An absolute timeout does not
drift.** It needs no extra tick, since the deadline is a fixed point, so a loop that sleeps until
the next multiple of its period wakes exactly on it:

```c
for (int64_t next = period_ms; ; next += period_ms) {
	k_sleep(K_TIMEOUT_ABS_MS(next));
	do_work();
}
```

If the deadline has already passed when the thread goes to sleep, the thread wakes at the next
tick: `z_add_timeout()` waits `max(1, dticks)` ticks for an absolute timeout. A periodic thread
that falls behind therefore catches up one tick at a time, rather than all at once.

### Busy waiting
`k_busy_wait(usec)` keeps the CPU busy for a number of microseconds, without giving it up. It is
for delays too short for a tick, such as the few microseconds a peripheral needs between two
register writes, and is almost never the right tool for anything longer. Two properties matter
later. It measures elapsed time, not time spent running, so a thread preempted during a busy wait
does not get the lost time back. And, unlike an ordinary loop, it lets simulated time pass on
`native_sim`.

---

## Kernel timers
A **kernel timer** calls a function when a timeout expires, once or periodically:

```c
static void sample_expired(struct k_timer *timer);

K_TIMER_DEFINE(sample_timer, sample_expired, NULL);

k_timer_start(&sample_timer, K_MSEC(100), K_MSEC(100));
```

`k_timer_start()` takes the delay to the first expiry and the period after it; a period of
`K_NO_WAIT` makes the timer one-shot. `k_timer_stop()` stops it. The two functions given to
`K_TIMER_DEFINE()` are both optional:
* The **expiry function** runs *in the system clock's interrupt handler*, every time the timer
  expires. It is interrupt context, with every restriction the next section lists.
* The **stop function** runs when the timer is stopped with `k_timer_stop()` while running, in the
  context of whoever called `k_timer_stop()`.

A thread can also wait for a timer instead of giving it a function: `k_timer_status_sync()` blocks
until the timer expires, and returns at once if it has already expired since its status was last
read, or is stopped. `k_timer_status_get()` returns how many times the timer has expired since
then, without blocking. Both reset the count.

**Timers do not add the extra tick.** `k_timer_start()` subtracts one tick from a relative delay
before handing it to `z_add_timeout()`, which adds it back, and every period is counted from the
previous expiry rather than from when the expiry function happened to run. The comment in
`deps/zephyr/kernel/timer.c` explains that the period "is always guaranteed to be reset from within
the timer ISR, so no round up is desired and 1 is subtracted in there". That is why a 500 ms
one-shot timer started at 0 ms expires at exactly 500 ms, and a periodic 100 ms timer at 100, 200
and 300 ms, with no drift, while `k_msleep(500)` returns at 510 ms.

---

## Interrupt context
Code in an interrupt handler runs *on top of* whatever thread was running, on a separate stack
(`CONFIG_ISR_STACK_SIZE`, 2048 bytes by default). It is not a thread, so **it must never block or
sleep**: there is no thread to put to sleep, and the thread underneath has nothing to do with it.
`k_is_in_isr()` tells code which context it is in.

What that means in practice:
* A kernel function may only be called from an ISR if its documentation says so. In
  `deps/zephyr/include/zephyr/kernel.h`, those carry the `@isr_ok` tag: `k_sem_give()`,
  `k_work_submit()`, `k_msgq_put()` and many more.
* A function that can wait may be called from an ISR only with `K_NO_WAIT`, as
  `k_sem_take()`'s documentation says: *"`timeout` must be set to `K_NO_WAIT` if called from ISR"*.
* **Mutexes may not be used from ISRs at all**: a mutex has an owner, and an ISR is not a thread
  that could own it.
* An ISR should be short, because while it runs, no thread runs.

Breaking the rule is not always caught. A `k_msleep()` in a timer's expiry function builds and, on
`native_sim`, even appears to work. With **`CONFIG_ASSERT=y`**, the kernel checks, and the same
program stops at once:

```text
ASSERTION FAIL [!arch_is_in_isr()] @ WEST_TOPDIR/deps/zephyr/kernel/sched.c:1222
```

Enable assertions while developing: they cost flash and time, and catch exactly this kind of
mistake.

On a board, a peripheral's interrupt handler is connected with `IRQ_CONNECT()`, which the driver
usually does for you. In this course the interrupt handler you will meet most is the system
clock's, through timer expiry functions, and on `native_sim` the emulated GPIO controller's
callbacks run in the context of whoever changed the pin, as L02 showed.

---

## Work queues
Interrupt handlers must be short and may not block, so real work is moved out of them: into a
thread. A **work queue** is a thread with a queue of **work items**, each a function to call. An ISR
submits an item, and the work queue's thread calls its function, soon after, in thread context,
where it may sleep, block, lock a mutex and log.

### Work items

```c
static void sample_handler(struct k_work *work);

K_WORK_DEFINE(sample_work, sample_handler);

k_work_submit(&sample_work);
```

`k_work_submit()` puts the item on the **system work queue** and is safe to call from an ISR. It
returns:

| Return value | Meaning |
|--------------|---------|
| 1 | The item was idle, and is now queued. |
| 0 | The item was already queued. Nothing changes: it stays in its place and runs **once**. |
| 2 | The item was running, and is now queued again, to the queue it is running on. |
| negative | An error, such as a queue that has not been started. |

The second row is a property to design around: **a work item is not a counter**. Submitting it
three times before it runs gives one call. If every event matters, the events must be counted or
queued somewhere else, and the handler must process all of them.

A handler receives only the `struct k_work` pointer. To give it context, embed the `k_work` in a
larger structure and recover the structure with `CONTAINER_OF()`.

### The system work queue
Zephyr starts one work queue for everyone, in a thread named `sysworkq`:

| Option | Default |
|--------|---------|
| `CONFIG_SYSTEM_WORKQUEUE_PRIORITY` | -1: the lowest cooperative priority |
| `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` | 1024 |
| `CONFIG_SYSTEM_WORKQUEUE_NO_YIELD` | Not set: the queue yields between items |

Being cooperative, a handler on the system work queue is never preempted by another thread, and
every subsystem that uses the queue waits for it. **Keep handlers there short, and never block in
them for long.** The Kconfig help says why the queue still yields between items: to keep a long
queue from starving other threads.

### Work queues of your own
Work that is slow, or that must not wait behind other subsystems' work, gets a queue of its own:

```c
K_THREAD_STACK_DEFINE(store_stack, 1024);
static struct k_work_q store_queue;

const struct k_work_queue_config config = {.name = "store_q"};

k_work_queue_start(&store_queue, store_stack, K_THREAD_STACK_SIZEOF(store_stack), 5, &config);
k_work_submit_to_queue(&store_queue, &store_work);
```

The queue's priority is its thread's priority, and may be preemptible. `struct k_work_queue_config`
also has `no_yield` and `essential`.

### Delayable work
A **delayable** work item is submitted after a delay, through a timeout inside it:

```c
K_WORK_DELAYABLE_DEFINE(idle_work, idle_handler);

k_work_schedule(&idle_work, K_MSEC(250));     /* If not already scheduled: in 250 ms. */
k_work_reschedule(&idle_work, K_MSEC(250));   /* In 250 ms from now, whatever was scheduled. */
```

The two differ exactly where it matters:
* **`k_work_schedule()`** does nothing if the item is already scheduled or queued: the first
  deadline wins. It returns 0 in that case.
* **`k_work_reschedule()`** replaces any deadline with the new one: the last call wins.

`k_work_reschedule()` is therefore the tool for a timeout that should expire only once events *stop*
coming, such as an inactivity timeout or a debounce. `k_work_cancel_delayable()` cancels a scheduled
item, and the `_sync` variants of the cancel functions also wait for a running handler to finish.
Inside a handler, `k_work_delayable_from_work()` turns the `struct k_work *` back into the delayable
item. The [`timer_work`](../examples/timer_work) example uses all three kinds of queue.

---

## ISR, work item or thread?

| Use | For | Because |
|-----|-----|---------|
| **An ISR**, or a timer's expiry function | Recording what happened, giving a semaphore, submitting work. Microseconds. | Nothing else can react before the next thread runs. |
| **A work item** on the system work queue | Short follow-up work that may block briefly: reading a register over I2C, logging. | No thread and stack of its own to pay for. |
| **A work queue of your own** | Follow-up work that is slow, or must not wait for other subsystems. | Its own priority, its own stack, and nobody else's work in the way. |
| **A thread** | Work with state that lives across many events, such as a protocol or a control loop, and waits in more than one place. | A thread keeps its place in its own code between events; a handler starts from the top every time. |

---

## native_sim's time model
`native_sim` advances simulated time only when the CPU is idle, that is, when every thread is
blocked or sleeping. Code that runs takes **no** simulated time. Everything above behaves as on a
microcontroller as long as threads wait with kernel calls, and the traces of this lecture's
examples are exact for that reason. Three things do not:

**A busy loop stops the world.** A thread that waits with `while (!flag) {}` for an interrupt to set
`flag` never lets the CPU go idle, so simulated time never moves, the interrupt never arrives, and
nothing else ever runs. On a microcontroller the timer interrupt would preempt the loop after the
right time, and the program would merely waste power. The [`busy_loop`](../examples/busy_loop)
example shows both. The fix is the same on both: **block on a kernel object**, such as a semaphore
the ISR gives, or the timer itself with `k_timer_status_sync()`. Zephyr's documentation of the
architecture (`deps/zephyr/boards/native/doc/arch_soc.rst`) suggests `k_cpu_idle()` inside such a
loop as a last resort.

**A stuck program ignores the first `Ctrl+C`.** `native_sim` handles `SIGINT` by asking the
simulation to stop at its next step, and a stuck simulation takes no next step. A second `Ctrl+C`
ends it. For the same reason `timeout 5 ./zephyr.exe` does not stop a stuck program, and
`timeout -s KILL 5 ./zephyr.exe` does. The course's test runner kills a program that runs for
30 seconds of real time and reports it as a probable busy loop.

**Preemption and time slices happen only at kernel calls and during `k_busy_wait()`.** An interrupt
can only arrive when simulated time moves, so a preemptible thread running ordinary code is never
interrupted, and a loop of a million iterations is never sliced. `k_busy_wait()` does move time,
and during it everything works: a thread that wakes preempts, and two preemptible threads of equal
priority busy-waiting 50 ms each take turns every 20 ms, one time slice. This lecture's exercises
use `k_busy_wait()` wherever a thread has to stand for work that takes CPU time.

---

## Stacks
Every thread has a stack of the size its definition gave, and nothing stops a thread from using
more: it writes past the end, into whatever lies there, usually another thread's stack or kernel
data. Stack overflow is one of the most common causes of crashes that make no sense in an RTOS
application, and stack sizes are one of the most common things to get wrong.

The stacks the kernel creates have Kconfig options. On `native_sim/native/64`:

| Option | Value |
|--------|-------|
| `CONFIG_MAIN_STACK_SIZE` | 1024 |
| `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` | 1024 |
| `CONFIG_ISR_STACK_SIZE` | 2048 |
| `CONFIG_IDLE_STACK_SIZE` | 256 |

### On a board
On a microcontroller, three tools find stack problems at run time:
* The **thread analyzer** (`CONFIG_THREAD_ANALYZER`) fills each stack with a known pattern at
  creation and reports how much of it has been overwritten: each thread's high-water mark.
* The **stack sentinel** (`CONFIG_STACK_SENTINEL`) stores a magic value at the far end of each
  stack, and checks it at every context switch, among other places.
* **Hardware stack protection** (`CONFIG_HW_STACK_PROTECTION`) uses the MPU to fault on the first
  write past the stack.

### On native_sim
**None of them works on `native_sim`**, for one reason: a `native_sim` thread does not run on the
stack Zephyr gave it. It runs on the stack of the Linux thread that backs it, which is megabytes
deep, and Zephyr's stack only holds a little bookkeeping. The help text of
`CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE`, in `deps/zephyr/arch/posix/Kconfig`, says so, and the
thread analyzer's Kconfig says `depends on !ARCH_POSIX`. Asking for it aborts the build:

```text
warning: THREAD_ANALYZER (defined at subsys/debug/thread_analyzer/Kconfig:6) was assigned the value
'y' but got the value 'n'. Check these unsatisfied dependencies: (!ARCH_POSIX) (=n).
```

The documentation (`deps/zephyr/boards/native/doc/arch_soc.rst`) lists stack canaries and hardware
stack protection as unsupported too. The stack sentinel does build, but it guards a stack the thread
never uses: the [`stack_usage`](../examples/stack_usage) example, whose thread needs twice its stack,
runs without complaint with `CONFIG_STACK_SENTINEL=y` as well as without.

### Static stack analysis
What works everywhere is asking the compiler. **`CONFIG_STACK_USAGE=y`** compiles every file with
GCC's `-fstack-usage`, which writes, beside each object file, a `.su` file with the stack frame
size of every function in it:

```text
.../src/main.c:18:13:print_report	2096	dynamic,bounded
.../src/main.c:27:13:reporter_entry	16	static
```

A frame is one function's own need, not including the functions it calls. **A thread needs the sum
of the frames along its deepest call chain**, plus, on targets where interrupts run on the
interrupted thread's stack, what an interrupt pushes onto it. The `.su` files give the frames; working out the chains is up
to you, or to tools built on the same data. The numbers depend on the architecture and the compiler
options, so `native_sim`'s x86-64 frames are a method to practise, not numbers to copy into a
board's configuration.

---

## Summary
* A thread is an entry function, a stack, a priority and a `struct k_thread`. Define long-lived
  threads with `K_THREAD_DEFINE()`; the kernel starts them just before `main()`, with the scheduler
  locked until all of them are started.
* Lower numbers are higher priorities. Negative priorities are cooperative, and a cooperative thread
  keeps the CPU until it gives it up. A preemptible thread loses it as soon as a higher-priority
  thread is ready.
* The scheduler acts when a thread blocks, sleeps or returns, when a thread or an interrupt makes a
  higher-priority thread ready, at `k_yield()`, and at the end of a time slice.
* Time is kept in ticks. A relative timeout is rounded up and gets one extra tick; an absolute
  timeout does not, which makes it the tool for periodic loops. Timers compensate for the extra
  tick, and do not drift.
* A timer's expiry function runs in interrupt context: no sleeping, no blocking, no mutexes, and
  `K_NO_WAIT` for anything that can wait. `CONFIG_ASSERT=y` catches the mistake.
* Defer work from ISRs to work queues. A work item submitted while queued runs once. Use
  `k_work_reschedule()` for a timeout that restarts with every event.
* On `native_sim`, code takes no simulated time: a busy loop stalls everything, and preemption only
  happens at kernel calls and during `k_busy_wait()`.
* Stack sizes are unchecked on `native_sim`. Use `CONFIG_STACK_USAGE` there, and the thread analyzer
  on a board.

---
