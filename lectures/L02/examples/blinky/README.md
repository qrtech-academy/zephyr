# Blinky
The board's LED, found through the devicetree alias `led0`, blinked from a loop. There is no LED
on `native_sim` to look at, so the program asks the emulated GPIO controller what the pin is
driving, and logs it.

```bash
west build -p -b native_sim/native/64 lectures/L02/examples/blinky
west build -t run
```

Or for two simulated seconds, without waiting for them:

```bash
./build/zephyr/zephyr.exe -stop_at=2 -no-rt
```

---

## What to look at
* **`led0` is defined by the board**, in `deps/zephyr/boards/native/native_sim/native_sim.dts`: an
  alias pointing at `led_0`, whose `gpios` property is `<&gpio0 0 GPIO_ACTIVE_HIGH>`.
  `GPIO_DT_SPEC_GET()` turns that property into a device pointer, a pin and flags at compile time.
* **`build/zephyr/zephyr.dts`** is the board's devicetree after every overlay has been applied.
  It is the file to read when you want to know what the build actually saw.
* **`gpio_emul_output_get()`** exists only on the emulated controller. It is the one line of this
  program that would not build for a real board.

---

## Live demo: two ways to get a device wrong
Both use this example unchanged, and both produce an error message that names a devicetree node
only by a number.

**1. The node is disabled.** [`broken.overlay`](./broken.overlay) sets `status = "disabled"` on
`gpio0`, the controller the LED is wired to:

```bash
west build -p -b native_sim/native/64 lectures/L02/examples/blinky -- \
    -DEXTRA_DTC_OVERLAY_FILE=broken.overlay
```

The build fails while compiling `main.c`:

```text
.../deps/zephyr/include/zephyr/device.h:96:41: error: ‘__device_dts_ord_32’ undeclared here (not in a function); did you mean ‘__device_dts_ord_38’?
```

A device object is only declared for nodes whose status is okay, so `GPIO_DT_SPEC_GET()` names a
device that does not exist. The *did you mean* is noise: 38 is simply the nearest number that
does exist. To find out what 32 is, look at the list of nodes in their dependency order at the
top of `build/zephyr/include/generated/zephyr/devicetree_generated.h`:

```text
 *   32  /gpio_emul
```

CMake also hinted at it earlier, with a warning that is easy to scroll past:
`No SOURCES given to Zephyr library: drivers__gpio`. With no enabled GPIO controller, no GPIO
driver was built.

**2. The node is enabled, but its driver is not built.** Turn the GPIO subsystem off:

```bash
west build -p -b native_sim/native/64 lectures/L02/examples/blinky -- -DCONFIG_GPIO=n
```

This time `main.c` compiles, because the device is declared, and the *linker* fails:

```text
undefined reference to `__device_dts_ord_32'
```

The declaration came from the devicetree, but the definition would have come from the driver,
and Kconfig left the driver out. Same number, same node, a different mistake: *undeclared* means
look at the devicetree, *undefined reference* means look at Kconfig.

---
