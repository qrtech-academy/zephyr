# Threads
Five threads and `main()`, each waiting on a semaphore until it is its turn, and a trace of who runs
when. Act 1 shows that a cooperative thread keeps the CPU until it gives it up, act 2 that a
preemptible thread loses the CPU the moment a higher-priority thread becomes ready, and act 3 that
`k_yield()` only hands the CPU to a thread of the same or higher priority.

```bash
west build -p -b native_sim/native/64 lectures/L03/examples/threads
west build -t run
```

Or, to stop by itself:

```bash
./build/zephyr/zephyr.exe -stop_at=1 -no-rt
```

The trace, with each line's thread and priority:

```text
-- Act 1: cooperative threads
main   ( 10)  waking coop_a
coop_a ( -1)  running
coop_a ( -1)  gave coop_b's semaphore: coop_b has a higher priority, but I keep the CPU
coop_b ( -2)  running, because coop_a yielded
coop_a ( -1)  back again, after coop_b returned
-- Act 2: preemption
main   ( 10)  waking low
low    (  5)  running
low    (  5)  giving high's semaphore
high   (  2)  running, in the middle of low's k_sem_give()
low    (  5)  back again: high preempted me inside k_sem_give()
-- Act 3: k_yield()
main   ( 10)  waking low
low    (  5)  running
low    (  5)  gave peer's semaphore: peer has my priority, so it waits
peer   (  5)  running, because low yielded
low    (  5)  back again, after peer returned
low    (  5)  yield returned at once: main is ready, but has a lower priority
main   ( 10)  done: every other thread has returned
```

---

## What to look at
* **Lower numbers are higher priorities.** Negative priorities are cooperative, from 0 upwards
  preemptible. `CONFIG_MAIN_THREAD_PRIORITY=10` in [`prj.conf`](./prj.conf) puts `main()` below every
  thread it wakes, so each `k_sem_give()` in `main()` returns only after the woken thread blocks or
  returns.
* **Every thread is created at boot by `K_THREAD_DEFINE`**, whose last argument, 0, starts it
  without delay. Each one blocks on its semaphore as soon as it first runs, and stays blocked until
  its semaphore is given.
* **Make `coop_a` preemptible** by changing its priority from `-1` to `1`. `coop_b` then runs inside
  `k_sem_give()`, before `coop_a` prints its second line, exactly as `high` does in act 2.
* **The trace is identical on every run.** Nothing here depends on time: every switch happens at a
  kernel call, which is also the only place a switch *can* happen on `native_sim`. On a real board,
  a timer interrupt could preempt a preemptible thread anywhere; see the `busy_loop` example.

---
