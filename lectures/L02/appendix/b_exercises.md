# Appendix B

## Exercises
These exercises reinforce the concepts from [Appendix A](./a_devicetree.md).

> **How to check your work.** Write each exercise set as its own application under
> [`lectures/L02/exercises`](../exercises/README.md), with the file names given there, and run
> `make test` from the repository root. A set's tests switch on as soon as its files exist, and a
> set you have not started is reported as `SKIP`. Each application grows through its set, and the
> tests check where it ends up. Every set is built twice: once as you wrote it, and once with an
> extra overlay from the test suite merged on top, which moves pins, turns polarities around, and
> adds or removes nodes. A program that takes its hardware from the devicetree prints the second
> expected output without a change; a program with a pin number or a polarity written into its C
> code does not.

Remember `source env.sh` in every new terminal. Build and run an exercise from the repository root:

```bash
west build -p -b native_sim/native/64 lectures/L02/exercises/exercise1
west build -t run
```

To build with the same extra overlay as the tests, from the repository root:

```bash
west build -p -b native_sim/native/64 lectures/L02/exercises/exercise1 -- \
    -DEXTRA_DTC_OVERLAY_FILE=$PWD/lectures/L02/exercises/test/exercise1-extra.overlay
```

Print with `printk()` in every set, so that the output has no timestamps other than the ones the
exercise asks for.

---

# Exercise Set 1 – The board's LED
Create an application named `exercise1` to work in for this exercise set, with a `CMakeLists.txt`
and `src/main.c` as in L01, and a `prj.conf` that enables GPIO (`CONFIG_GPIO=y`).

## Exercise 1.1 – Finding the LED
In this exercise you will get the board's LED from the devicetree.

### Tasks
* In `main.c`, get a `struct gpio_dt_spec` for the `gpios` property of the node with the alias
  `led0`, at file scope.
* In `main()`, check that the LED's controller is ready. If it is not, print
  `GPIO controller <name> is not ready` and return.
* Print the LED's pin, its controller's name, and whether it is active high or active low, from the
  spec's flags:

  ```text
  LED: pin 0 on gpio_emul, active high
  ```

* Configure the pin as an output, initially inactive.

### Reflection
* Find `led0` in `deps/zephyr/boards/native/native_sim/native_sim.dts`. Which line is the alias,
  and which is the label? Which of the two did your code use?
* Where does the name `gpio_emul` come from?
* What would the same program print on a board whose `led0` is declared
  `<&gpio1 5 GPIO_ACTIVE_LOW>`, and what in your code would change?

---

## Exercise 1.2 – Blinking as the devicetree says
### Tasks
* Create `app.overlay` in `exercise1`, adding a `zephyr,user` node under the root with two
  properties:
  * `blink-ms = <200 300>;`: how long the LED is on, and then off, in milliseconds.
  * `blinks = <3>;`: how many times to blink.
* In `main.c`, read `blink-ms` into a `static const int` array at file scope, and use `blinks`
  directly where you need it.
* After configuring the pin, blink the LED `blinks` times. For each blink:
  * Turn the LED on with `gpio_pin_set_dt()`, print `LED on, pin <level> at <uptime> ms`, and sleep
    for the on time.
  * Turn the LED off, print `LED off, pin <level> at <uptime> ms`, and sleep for the off time.

  `<level>` is `high` or `low`: what the emulated controller is driving on the pin, from
  `gpio_emul_output_get()` in `<zephyr/drivers/gpio/gpio_emul.h>`.
* Finally print `Done after <blinks> blinks`.

The output must be:

```text
LED: pin 0 on gpio_emul, active high
LED on, pin high at 0 ms
LED off, pin low at 210 ms
LED on, pin high at 520 ms
LED off, pin low at 730 ms
LED on, pin high at 1040 ms
LED off, pin low at 1250 ms
Done after 3 blinks
```

The tests build the application a second time with
[`exercise1-extra.overlay`](../exercises/test/exercise1-extra.overlay), which points `led0` at a new,
active-low LED on pin 5, and blinks it twice, for 50 and 150 ms. The output must then be:

```text
LED: pin 5 on gpio_emul, active low
LED on, pin low at 0 ms
LED off, pin high at 60 ms
LED on, pin low at 220 ms
LED off, pin high at 280 ms
Done after 2 blinks
```

**Hint:** `DT_PATH(zephyr_user)` is the node identifier of `/zephyr,user`. An array property is an
initializer: `static const int blink_ms[] = DT_PROP(node, blink_ms);`.

### Reflection
* You wrote no binding for `zephyr,user`. How did the build know the types of its properties?
* Remove `blinks` from the overlay and build. Explain the error. Then make the program blink once
  when `blinks` is missing, without adding it back.
* In the second build, the LED is *on* while the pin is *low*. Which line of your code decided
  that, and why did it not need to change?
* In the second build, find `blink-ms` in `build/zephyr/zephyr.dts`. How does the file say which
  overlay the value came from, and why is it not `app.overlay`, which also sets it?

---

# Exercise Set 2 – A button of your own
Create an application named `exercise2` to work in for this exercise set, with the same
`CMakeLists.txt` and `prj.conf` as `exercise1`.

## Exercise 2.1 – Describing the hardware
In this exercise you will add hardware the board does not have, with an overlay.

### Tasks
Create `app.overlay` in `exercise2`, which adds:
* A second LED, as a child of the board's existing `leds` node: pin 2 of `gpio0`, active high, with
  the node label `led1`.
* A node named `buttons` under the root, with `compatible = "gpio-keys"`, holding one button: pin 1
  of `gpio0`, active low with a pull-up, with the node label `button0`.
* The aliases `led1` for the new LED and `sw0` for the button.

Use exactly these node labels: the test suite's overlay refers to them.

### Reflection
* Why does the new LED need no `compatible` property of its own?
* Build the application and open `build/zephyr/zephyr.dts`. What are the button's flags, in hex,
  and how are they made up?

---

## Exercise 2.2 – The callback
### Tasks
In `main.c`:
* Get `gpio_dt_spec`s for the aliases `led1` and `sw0`, and check that both are ready.
* Print `Button on pin <pin>, LED 1 on pin <pin>`.
* Configure LED 1 as an output, initially inactive, and the button as an input.
* Configure an interrupt on the button for when it becomes **active**, and add a callback for it.
* In the callback, count the press, toggle a `bool` that says whether LED 1 is on, set LED 1 with
  `gpio_pin_set_dt()`, and print:

  ```text
  Press <count> at <uptime> ms, LED 1 <on|off>, pin <high|low>
  ```

  where the pin's level is read back with `gpio_emul_output_get()`.

---

## Exercise 2.3 – The finger
### Tasks
Nobody can press an emulated button, so `main()` does it:
* Write a function `static void press(bool pressed)` that sets the button's input with
  `gpio_emul_input_set()`. It takes whether the button is pressed, and must work out the **level
  on the pin** from the button's flags.
* After setting up the callback, `main()` presses the button three times. Before each press it
  sleeps for the next value of `static const int gap_ms[] = {100, 200, 200};`, then presses, sleeps
  50 ms, and releases.
* Finally print `<count> presses counted`.

The output must be:

```text
Button on pin 1, LED 1 on pin 2
Press 1 at 110 ms, LED 1 on, pin high
Press 2 at 380 ms, LED 1 off, pin low
Press 3 at 650 ms, LED 1 on, pin high
3 presses counted
```

The second build merges [`exercise2-extra.overlay`](../exercises/test/exercise2-extra.overlay), which
moves the button to pin 7 as an active-high button with a pull-down, and LED 1 to pin 3 as an
active-low LED. The output must then be:

```text
Button on pin 7, LED 1 on pin 3
Press 1 at 110 ms, LED 1 on, pin low
Press 2 at 380 ms, LED 1 off, pin high
Press 3 at 650 ms, LED 1 on, pin low
3 presses counted
```

**Hint:** An active-low button is pressed when its pin is low.

### Reflection
* Make `press()` always drive the pin low for a press, and run the second build. At what times are
  the presses counted now, and why?
* Print `k_is_in_isr()` in the callback. What does it print, and what would the same callback run
  in on a microcontroller? What does that forbid in the callback?
* Nothing happens when the finger releases the button. Why not?
* The button's input level is high before the first press, although nothing in your code set it.
  What did?
* Build with `-DDTC_OVERLAY_FILE=` and the path to `exercise2-extra.overlay`, instead of
  `-DEXTRA_DTC_OVERLAY_FILE=`. What happens, and why?

---

# Exercise Set 3 – A binding of your own
Create an application named `exercise3` to work in for this exercise set, with the same
`CMakeLists.txt` and `prj.conf` as `exercise1`.

## Exercise 3.1 – The binding
In this exercise you will define a kind of node of your own.

### Tasks
Create `dts/bindings/qacademy,alarm.yaml` in `exercise3`: a binding for the compatible
`qacademy,alarm`, a temperature alarm with an LED, which includes `base.yaml` and has these
properties:

| Property | Type | Required | Meaning |
|----------|------|----------|---------|
| `alarm-name` | string | yes | Human-readable name. |
| `threshold` | int | yes | The temperature, in degrees Celsius, at which the alarm goes on. |
| `hysteresis` | int | no, default 2 | How many degrees below the threshold the temperature must fall before the alarm goes off. |
| `severity` | string | yes | One of `info`, `warning` or `critical`, in that order. |
| `led-gpios` | phandle-array | yes | The LED that shows the alarm. |

Create `dts/bindings/vendor-prefixes.txt`, registering the vendor prefix `qacademy` (the prefix, a
tab, and a vendor name).

---

## Exercise 3.2 – The alarms
### Tasks
Create `app.overlay` in `exercise3`, adding three nodes under the root, in this order. Every
alarm's LED is on `gpio0` and active high, and `alarm-fan` is the only node with
`status = "disabled"`:

| Node name | Label | `alarm-name` | `threshold` | `hysteresis` | `severity` | LED pin |
|-----------|-------|--------------|-------------|--------------|------------|---------|
| `alarm-overheat` | `overheat` | `Overheat` | 70 | not set | `critical` | 10 |
| `alarm-warm` | `warm` | `Warm` | 45 | 5 | `warning` | 11 |
| `alarm-fan` | `fan` | `Fan` | 25 | not set | `info` | 12 |

### Reflection
* Remove `severity` from one node and build. Then set it to `"fatal"` and build. Which step of the
  build reports each error, and has any C been compiled yet?
* Delete `vendor-prefixes.txt` and build. What changes?

---

## Exercise 3.3 – The table
### Tasks
In `main.c`:
* Define a `struct alarm` holding the name, threshold, hysteresis, severity (as an `int`), a
  `struct gpio_dt_spec` for the LED, and a `bool` saying whether the alarm is on.
* Build an array of them, `alarms`, with **one entry for each enabled `qacademy,alarm` node**, using
  `DT_FOREACH_STATUS_OKAY()`. Store the severity as its index in the binding's `enum`, and keep a
  table of the names `"info"`, `"warning"` and `"critical"` to print it.
* Print `<count> alarms`, and then, for each alarm:

  ```text
  <name>: <severity> at <threshold> C, hysteresis <hysteresis>, LED pin <pin>
  ```

  and configure its LED as an output, initially inactive.
* Go through the readings `static const int readings[] = {20, 50, 42, 75, 66, 30};`. For each,
  print `Reading <t> C`, and then for each alarm, in table order:
  * An alarm that is off goes on if the reading is at or above its threshold.
  * An alarm that is on goes off if the reading is below its threshold minus its hysteresis.
  * When an alarm changes, set its LED, and print, indented by two spaces:

    ```text
      <name> (<severity>) <on|off>, pin <pin> <high|low>
    ```

The output must be:

```text
2 alarms
Overheat: critical at 70 C, hysteresis 2, LED pin 10
Warm: warning at 45 C, hysteresis 5, LED pin 11
Reading 20 C
Reading 50 C
  Warm (warning) on, pin 11 high
Reading 42 C
Reading 75 C
  Overheat (critical) on, pin 10 high
Reading 66 C
  Overheat (critical) off, pin 10 low
Reading 30 C
  Warm (warning) off, pin 11 low
```

The second build merges [`exercise3-extra.overlay`](../exercises/test/exercise3-extra.overlay), which
deletes the `warm` node, enables `fan`, and changes `overheat`'s threshold to 60 and hysteresis to
10. The output must then be:

```text
2 alarms
Overheat: critical at 60 C, hysteresis 10, LED pin 10
Fan: info at 25 C, hysteresis 2, LED pin 12
Reading 20 C
Reading 50 C
  Fan (info) on, pin 12 high
Reading 42 C
Reading 75 C
  Overheat (critical) on, pin 10 high
Reading 66 C
Reading 30 C
  Overheat (critical) off, pin 10 low
```

**Hint:** `DT_ENUM_IDX(node_id, severity)` is the index of the node's `severity` in the binding's
`enum`. The compatible, written as a C token, is `qacademy_alarm`.

### Reflection
* Overheat's hysteresis is 2, although its node does not set it. Where does the 2 come from, and
  what would happen without it in the binding?
* In what order are the alarms in the table? Find the list in
  `build/zephyr/include/generated/zephyr/devicetree_generated.h`. Can a program rely on that order?
* `alarm-fan` is disabled in the first build. Which instance number does it have, and does
  `DT_NODE_EXISTS(DT_NODELABEL(fan))` give 1 or 0?
* Add `BUILD_ASSERT(DT_PROP(DT_NODELABEL(warm), hysteresis) == 5);` to `main.c`, and run the second
  build. What happens, and why is the error not an assertion failure?
* The table is not `const`, because each alarm's `active` changes. Which fields could be `const`,
  and how would you split the struct so that they are?

---

# Exercise Set 4 – Reading the devicetree
No code in this set: answer the questions by looking. Build `exercise1` and `exercise2` into
separate build directories first, e.g. with `-d build/exercise1` and `-d build/exercise2`.

## Exercise 4.1 – The merged tree
### Tasks
Answer, using `zephyr.dts` in the `exercise2` build:
* Which file and line defined the alias `sw0`? And the alias `led0`?
* What does `/chosen` say the console is, and what is the node's compatible?

---

## Exercise 4.2 – The generated header
### Tasks
Answer, using `devicetree_generated.h`:
* What is the node identifier of the button in `exercise2`, and what does `DT_ALIAS(sw0)` expand
  to, step by step?
* What is the ordinal of `/gpio_emul` in the `exercise1` build, and in the `exercise2` build? Why
  can they differ?
* Which ordinals does the button's node depend on directly, and why those?

---

## Exercise 4.3 – Two errors, one number
### Tasks
* Build `exercise1` with `-DEXTRA_DTC_OVERLAY_FILE=` and the path to
  `lectures/L02/examples/blinky/broken.overlay`. Which error do you get, and which node does its
  number name?
* Build `exercise1` with `-DCONFIG_GPIO=n` instead. Which error do you get now, and what is the
  difference in what went wrong?

---
