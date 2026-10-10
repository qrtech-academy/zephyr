# Hello
The smallest useful Zephyr application: `printk()`, logging, sleeping, and the uptime clock.

```bash
west build -p -b native_sim/native/64 lectures/L01/examples/hello
west build -t run
```

Stop it with `Ctrl+C`. To stop by itself after three simulated seconds, and without waiting for them
in real time, run the program directly:

```bash
./build/zephyr/zephyr.exe -stop_at=3 -no-rt
```

---

## What to look at
* **The ticks are 510 ms apart, not 500.** `native_sim` has 100 ticks per second, and `k_sleep()`
  sleeps for *at least* the time asked for, rounded up to the next tick after the current one. Run
  it again: the numbers are the same, because time on `native_sim` is simulated.
* **The debug message never appears**, and is not in the binary at all. `LOG_MODULE_REGISTER(hello,
  LOG_LEVEL_INF)` sets this module's compile-time level. Change it to `LOG_LEVEL_DBG` and rebuild.
* **`build/zephyr/.config`** holds every Kconfig option the build ended up with: the board's
  defaults, then `prj.conf` on top. Search it for `CONFIG_LOG`.
* **`west build -t menuconfig`** browses the same options interactively. Press `/` to search.

---
