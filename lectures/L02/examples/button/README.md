# Button
A button that the board does not have, added with a devicetree overlay; an edge interrupt with a
callback that toggles the LED; and a thread that presses the emulated button, since nobody can.

```bash
west build -p -b native_sim/native/64 lectures/L02/examples/button
west build -t run
```

Or for three and a half simulated seconds, without waiting for them:

```bash
./build/zephyr/zephyr.exe -stop_at=3.5 -no-rt
```

---

## What to look at
* **[`app.overlay`](./app.overlay)** is found by its name alone; `CMakeLists.txt` does not mention
  it. It adds a `buttons` node and an `sw0` alias, the same alias Zephyr's own button sample uses.
* **`build/zephyr/zephyr.dts`** is the final, merged devicetree, and every node and property in it
  is annotated with the file and line it came from. Find `button_0`: its
  `gpios = < &gpio0 0x1 0x11 >` is pin 1 with flags `GPIO_PULL_UP | GPIO_ACTIVE_LOW` (`0x10 | 0x1`).
* **The C code only says *active*.** `GPIO_INT_EDGE_TO_ACTIVE` becomes a falling edge because the
  devicetree says active low. Change the flag in the overlay to `GPIO_ACTIVE_HIGH`, rebuild, and
  the LED toggles when the finger *releases* the button instead: press 1 moves from 0.910 s to
  1.020 s.
* **The callback reports `thread`.** The emulated GPIO controller runs callbacks in whichever
  context changed the pin, here the finger thread. On a microcontroller the same callback runs in
  an interrupt service routine. Press the button from a `k_timer` expiry function instead of a
  thread, and it reports `ISR`, because timer expiry functions run in the system clock's
  interrupt. Write the callback as if it were always an ISR: no sleeping, no blocking.
* **`atomic_inc()` returns the old value**, hence the `+ 1`. Remove it and the first press is
  press 0.

---
