# Bindings
A devicetree binding of the application's own, `qacademy,blink-pattern`, three nodes that use it,
and C code that turns every enabled node into an entry of a table at compile time. The program
never mentions a pattern by name: add a node to the overlay and the table grows.

```bash
west build -p -b native_sim/native/64 lectures/L02/examples/bindings
west build -t run
```

Or for ten simulated seconds, without waiting for them:

```bash
./build/zephyr/zephyr.exe -stop_at=10 -no-rt
```

---

## What to look at
* **[`dts/bindings/qacademy,blink-pattern.yaml`](./dts/bindings/qacademy,blink-pattern.yaml)** is
  the schema: property names, types, `required`, and `default`. It is found because the
  application directory's `dts/bindings` is always searched; nothing in CMake points at it.
* **[`dts/bindings/vendor-prefixes.txt`](./dts/bindings/vendor-prefixes.txt)** registers
  `qacademy`, the part of the compatible before the comma. Delete it and the build still
  succeeds, but warns `node '/pattern-sos' compatible 'qacademy,blink-pattern' has unknown vendor
  prefix 'qacademy'`. Twister treats that warning as an error.
* **[`app.overlay`](./app.overlay)** holds the data: three nodes, one of them disabled.
* **`build/zephyr/zephyr.dts`** shows the merged tree. Numbers come out in hex, and every
  property is annotated with the overlay line it came from.
* **`build/zephyr/include/generated/zephyr/devicetree_generated.h`** is what the C code actually
  reads. Every node gets a *node identifier*, a C token built from its path: `/pattern-sos` is
  `DT_N_S_pattern_sos` (`S` for a slash, `-` becomes `_`). Everything else is a macro named after
  it. Search the file for these:

  ```c
  #define DT_N_NODELABEL_sos                 DT_N_S_pattern_sos
  #define DT_N_INST_0_qacademy_blink_pattern DT_N_S_pattern_sos
  #define DT_N_S_pattern_sos_P_pattern_name "SOS"
  #define DT_N_S_pattern_sos_P_delays_ms_LEN 18
  #define DT_N_S_pattern_heartbeat_P_repeat 2
  #define DT_FOREACH_OKAY_qacademy_blink_pattern(fn) fn(DT_N_S_pattern_sos) fn(DT_N_S_pattern_heartbeat)
  ```

  `DT_NODELABEL(sos)`, `DT_PATH(pattern_sos)` and `DT_PROP(node_id, pattern_name)` are only ways
  of spelling these names. The last line is the whole of `DT_FOREACH_STATUS_OKAY()`: the disabled
  `pattern-strobe` is not in it, although its node, and a `DT_N_S_pattern_strobe_STATUS_disabled`
  macro, are.
* **`repeat` has a default** in the binding, so SOS, which does not set it, still has `repeat` 1.
* **The patterns take 3580 and 2080 ms, not 3400 and 2000.** Every `k_msleep()` rounds up by one
  10 ms tick: 18 sleeps for SOS, and 8 for Heartbeat's two rounds.
* **`BUILD_ASSERT()`** checks the devicetree against the code at compile time. Change `repeat` to
  3 in the overlay and the build stops with `heartbeat should play twice`.

---

## Live demo: a missing required property
Delete the `pattern-name` line from the `pattern-heartbeat` node in `app.overlay` and build. The
build stops before any C is compiled, with a message that names the property, the binding and the
node's line in the overlay:

```text
devicetree error: 'pattern-name' is marked as required in 'properties:' in '.../bindings/dts/bindings/qacademy,blink-pattern.yaml', but does not appear in <Node /pattern-heartbeat in .../bindings/app.overlay:18>
```

Devicetree errors caught by the binding look like this one, early and readable. The ones in the
[blinky](../blinky/README.md#live-demo-two-ways-to-get-a-device-wrong) demo are what gets past it.

---
