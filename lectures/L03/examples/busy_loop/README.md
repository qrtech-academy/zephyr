# Busy Loop
`main()` starts a 500 ms one-shot timer and waits for it to expire, either by blocking on a
semaphore that the expiry function gives, or, with `busy.conf`, by spinning on a flag that the
expiry function sets. On a microcontroller both work. On `native_sim` the busy loop never ends, and
this example exists to show why.

```bash
west build -p -b native_sim/native/64 lectures/L03/examples/busy_loop
./build/zephyr/zephyr.exe -stop_at=2 -no-rt
```

```text
0 ms: timer started, waiting for it to expire
blocking on a semaphore
500 ms: timer expired

Stopped at 2.010s
```

Now the busy loop:

```bash
west build -p -b native_sim/native/64 lectures/L03/examples/busy_loop -- -DEXTRA_CONF_FILE=busy.conf
./build/zephyr/zephyr.exe -stop_at=2 -no-rt
```

```text
0 ms: timer started, waiting for it to expire
spinning until the flag is set
```

And nothing more, although `-stop_at=2` asked for the program to stop after two simulated seconds.
**Press `Ctrl+C` twice** to end it.

---

## What to look at
* **No simulated time passes while code runs.** `native_sim` advances its clock only when the CPU
  goes idle, that is, when every thread is blocked or sleeping. The spinning thread never blocks, so
  the clock stays at 0 ms, the timer never expires, and the flag is never set. Not even `-stop_at`
  can end it, because it is checked by the same clock.
* **On a microcontroller the loop would end.** The timer is real hardware there: after 500 ms of
  spinning its interrupt preempts the loop, the expiry function sets the flag, and the loop exits.
  The program is still bad, since the CPU burns power for 500 ms for nothing, but it works.
  `native_sim` turns a wasteful program into a stuck one.
* **The first `Ctrl+C` does nothing.** `native_sim` handles `SIGINT` and `SIGTERM` by asking the
  simulation to stop at its next step, and a stuck simulation takes no next step. The handler
  removes itself, so the second signal ends the program. The same applies to `timeout`: plain
  `timeout 5 ./build/zephyr/zephyr.exe` does not stop it, but `timeout -s KILL 5` does.
* **The stuck program uses a whole host CPU core.** Watch it in `top` before you stop it.
* **The fix is the one a real target wants too:** block on a kernel object (here a semaphore) until
  the interrupt says there is something to do. Zephyr's documentation of the POSIX architecture
  (`deps/zephyr/boards/native/doc/arch_soc.rst`) describes the limitation, and suggests pausing a
  stuck program in a debugger to find the loop.

---
