# Timer and Work Queues
A periodic `k_timer` takes three samples, 100 ms apart. Its expiry function runs in interrupt
context, so it only submits a work item; the sampling itself runs in the system work queue's
thread, the slow storing in a dedicated work queue, and a delayable work item reports when the
samples have stopped coming.

```bash
west build -p -b native_sim/native/64 lectures/L03/examples/timer_work
west build -t run
```

Or, to stop by itself:

```bash
./build/zephyr/zephyr.exe -stop_at=1 -no-rt
```

The trace, with each line's uptime and the context it ran in:

```text
   0 ms  main      timer started
 100 ms  (ISR)     timer expired: submitting sample_work
 100 ms  sysworkq  sampling
 100 ms  store_q   storing, which sleeps for 30 ms
 140 ms  store_q   stored
 200 ms  (ISR)     timer expired: submitting sample_work
 200 ms  sysworkq  sampling
 200 ms  store_q   storing, which sleeps for 30 ms
 240 ms  store_q   stored
 300 ms  (ISR)     timer expired: submitting sample_work
 300 ms  sysworkq  sampling
 300 ms  sysworkq  last sample: timer stopped
 300 ms  store_q   storing, which sleeps for 30 ms
 340 ms  store_q   stored
 560 ms  sysworkq  idle timeout: no sample for 250 ms
```

---

## What to look at
* **The expiry function runs in an ISR**, and `k_is_in_isr()` says so. It may not sleep or block.
  Everything that may is moved into a work handler, which runs in an ordinary thread.
* **Three different contexts, one after another, at the same simulated millisecond.** Handing work
  from an ISR to a thread, and from one thread to another, takes no simulated time on `native_sim`.
* **`sysworkq` finishes its handler before `store_q` starts**, even though the work was submitted
  earlier: see the third sample. The system work queue's thread is cooperative (priority -1 by
  default), so nothing preempts it, which is also why a slow handler there holds up everyone else
  who uses it.
* **The idle timeout fires once, at 560 ms.** Each sample pushes it back with
  `k_work_reschedule()`, so it only expires 250 ms after the last one. Change it to
  `k_work_schedule()`, which leaves an already scheduled item alone, and it fires at 360 ms instead:
  250 ms after the *first* sample.
* **Sleeps last one tick longer than asked for.** 30 ms of sleep ends 40 ms later, and the 250 ms
  timeout expires after 260 ms. At 100 ticks per second a timeout is rounded up to whole ticks, and
  one more tick is added, because the tick in progress may already be partly over. The periodic
  timer does not drift: 100, 200, 300.

---
