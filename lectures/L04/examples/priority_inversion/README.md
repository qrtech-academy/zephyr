# Priority Inversion
Three threads and one lock. A low-priority thread takes the lock, a high-priority thread blocks on
it, and a medium-priority thread that needs no lock at all arrives and takes the CPU. Protected by a
semaphore, the high-priority thread waits for the medium one to finish. Protected by a mutex, it
does not, because the mutex lends its priority to the thread holding it.

```bash
west build -p -b native_sim/native/64 lectures/L04/examples/priority_inversion
./build/zephyr/zephyr.exe -stop_at=1 -no-rt
```

With a mutex instead of a semaphore:

```bash
west build -p -b native_sim/native/64 lectures/L04/examples/priority_inversion -- -DEXTRA_CONF_FILE=mutex.conf
./build/zephyr/zephyr.exe -stop_at=1 -no-rt
```

---

## What to look at
* **With the semaphore, `high waited 579 ms`.** Low computes from 0 to 31 ms, medium preempts it and
  computes until 531 ms, and only then does low get the CPU back to finish its remaining 69 ms. High
  has the highest priority in the system and waits the longest.
* **With the mutex, `high waited 79 ms`**: the rest of low's 100 ms of work, and nothing more. The
  trace prints `low (priority 5) gives the mutex back`. While high waited for the mutex, low ran at
  high's priority, so medium could not preempt it. After unlocking, low is back at priority 7 and
  finishes last.
* **The threads start at 21 and 31 ms**, not 20 and 30. A start delay, like a sleep, lasts at least
  the time asked for and ends on the tick after it.
* **`compute()` calls `k_busy_wait()` one millisecond at a time.** On `native_sim`, instructions
  take no simulated time, so a loop would finish instantly. `k_busy_wait()` moves the clock forward
  without giving up the CPU, and a thread that wakes up in the meantime can still preempt, which is
  what lets medium interrupt low here. Busy-waiting in 1 ms steps also means work that was
  interrupted does not count as done.

---
