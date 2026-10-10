# Appendix A
## Devicetree
This appendix covers how Zephyr describes hardware: the devicetree language, the board's tree and
the overlays that change it, the bindings that give it meaning, and the macros that turn it into C.
It ends with GPIO, the first device API the course uses, on `native_sim`'s emulated GPIO
controller. By the end you should be able to explain every file in the
[`blinky`](../examples/blinky), [`button`](../examples/button) and
[`bindings`](../examples/bindings) examples, and every devicetree error their READMEs show.

---

## Why hardware is data
An LED blinker for one board is easy to write: the LED is pin 13 of port B, so the code configures
pin 13 of port B. The trouble starts with the second board, where the LED is pin 5 of port A and
lights when the pin is *low*. The usual answer is a header of `#define`s per board, and a
`#ifdef` per difference, and it grows with every board and every peripheral.

Zephyr's answer is to describe the hardware in a separate language, **devicetree**, and to keep
that description out of the C code entirely. A board says, in its devicetree, *there is an LED, it
is on this pin of this controller, and it is active low*, and gives the LED a role name, `led0`.
The application asks for `led0`, and the build fills in the rest. The same C file then builds for
every board that has a `led0`, unchanged.

Devicetree comes from Open Firmware, by way of Linux, and the language is the same. What Zephyr
does with it is not. Linux compiles the tree into a binary blob that the kernel **parses at boot**.
Zephyr processes the tree **at build time**, into a C header of macros, and nothing of the tree
itself is left in the image. There is no lookup at run time, no parsing, and no memory spent on
it; there is also no way to change the hardware description without rebuilding.

A build uses two kinds of devicetree input:

| Input | What it says | Example |
|-------|--------------|---------|
| **Devicetree sources**: `.dts`, `.dtsi` and `.overlay` files | *What hardware there is*: nodes and their properties. | `led_0 { gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>; };` |
| **Bindings**: `.yaml` files | *What the properties mean*: which a node may have, their types, which are required. | `gpios: type: phandle-array, required: true` |

---

## The language
### Nodes and properties
A devicetree is a tree of **nodes**, each with **properties**. This is a piece of the devicetree of
`native_sim`, the board the course builds for, from
`deps/zephyr/boards/native/native_sim/native_sim.dts`:

```dts
/ {
	model = "Native Sim Board";
	compatible = "zephyr,posix";

	leds: leds {
		compatible = "gpio-leds";

		led0: led_0 {
			gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;
			label = "Green LED";
		};
	};

	i2c0: i2c@100 {
		status = "okay";
		compatible = "zephyr,i2c-emul-controller";
		clock-frequency = <I2C_BITRATE_STANDARD>;
		#address-cells = <1>;
		#size-cells = <0>;
		reg = <0x100 4>;
	};

	gpio0: gpio_emul {
		status = "okay";
		compatible = "zephyr,gpio-emul";
		rising-edge;
		falling-edge;
		high-level;
		low-level;
		gpio-controller;
		#gpio-cells = <2>;
	};
};
```

Reading it from the outside in:
* **`/`** is the root node. Every other node is a descendant, and is named by its **path**, like a
  file: `/leds/led_0`.
* **`i2c@100`** is a node name with a **unit address**, the part after `@`. It is the node's first
  `reg` address, and it makes names unique when there are several of a kind.
* **`gpio0:`** before a node is a **label**. Labels are how other parts of the tree, and the C code,
  refer to a node without spelling out its path.
* **`&gpio0`** in a property is a **phandle**: a reference to the node labelled `gpio0`.
* **`gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>`** is a *phandle-array*: a phandle followed by numbers the
  referenced node gives meaning to. `gpio0`'s `#gpio-cells = <2>` says each reference takes two:
  the pin, then the flags.
* **`status`** says whether a node is in use. `"okay"`, or no `status` at all, means enabled;
  `"disabled"` means the node is described, but nothing is built for it.
* **`compatible`** says what the node *is*. It selects the node's binding, and with it, the driver.
  The part before the comma names a vendor.

Property values come in a few forms:

| Form | Example | Meaning |
|------|---------|---------|
| Nothing | `rising-edge;` | A boolean: true if present. |
| String | `label = "Green LED";` | A string. |
| Cells | `reg = <0x100 4>;` | 32-bit numbers, in angle brackets. |
| Phandle | `zephyr,console = &uart0;` | A reference to another node. |
| Mixed cells | `gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;` | Phandles and numbers in one list. |
| String list | `compatible = "a,b", "c";` | Several strings, comma-separated. |
| Bytes | `mac = [00 11 22 33 44 55];` | 8-bit values, in square brackets. |

**`GPIO_ACTIVE_HIGH` is a C macro.** Devicetree files are run through the C preprocessor before
anything else reads them, so `native_sim.dts` can
`#include <zephyr/dt-bindings/gpio/gpio.h>`, where `GPIO_ACTIVE_LOW` is `(1 << 0)` and
`GPIO_PULL_UP` is `(1 << 4)`. The preprocessed file is `build/zephyr/zephyr.dts.pre`.

### Aliases and chosen
Two nodes directly under the root are special, and both hold only phandles:

```dts
chosen {
	zephyr,console = &uart0;
	zephyr,shell-uart = &uart0;
	zephyr,flash = &flash0;
};

aliases {
	led0 = &led0;
	i2c-0 = &i2c0;
};
```

* **`/aliases`** gives nodes *role names* that applications can rely on across boards. `led0` is
  the first LED, `sw0` the first button, wherever a board has them. Zephyr's samples, and this
  course's examples, find their hardware this way.
* **`/chosen`** tells *Zephyr itself* which node to use for a job: `zephyr,console` is where
  `printk()` goes.

A node therefore has up to three names: its **path** (`/leds/led_0`), which is unique; its
**labels** (`led0`), which belong to the source files and are for the tree to refer to itself;
and the **aliases** pointing at it (`led0`), which are the stable interface to applications. The
label and the alias here happen to be spelled the same; they are different things, and an overlay
can point the alias at another node without touching the label.

---

## The board's devicetree
A build starts from its board's devicetree. For `native_sim/native/64` that is
`deps/zephyr/boards/native/native_sim/native_sim_64.dts`, which is one line,
`#include "native_sim.dts"`, which in turn includes `posix.dtsi` and `skeleton.dtsi`. `.dtsi` files
are fragments shared between boards; a real SoC's `.dtsi` describes every peripheral on the chip,
mostly `status = "disabled"`, and each board enables the ones it wires up.

`native_sim`'s tree describes the simulated hardware, and the course uses much of it:

| Node | Label | Compatible | Used in |
|------|-------|------------|---------|
| `/gpio_emul` | `gpio0` | `zephyr,gpio-emul` | This lecture |
| `/leds/led_0` | `led0` | child of `gpio-leds` | This lecture |
| `/i2c@100` | `i2c0` | `zephyr,i2c-emul-controller` | L05 |
| `/flash-controller@0` | `flashcontroller0` | `zephyr,sim-flash` | L06 |
| `/uart` | `uart0` | `zephyr,native-pty-uart` | The console, and L06's shell |

---

## Overlays
An application changes its board's devicetree with **overlays**: devicetree fragments merged on top
of the board's tree before anything is generated from it. An overlay can add nodes, change
properties, and delete either:

```dts
/* Add nodes under the root. Nodes with the same path are merged, not replaced. */
/ {
	buttons {
		compatible = "gpio-keys";

		button0: button_0 {
			gpios = <&gpio0 1 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
			label = "User button";
		};
	};

	aliases {
		sw0 = &button0;
	};
};

/* Change an existing node, found by its label. */
&gpio0 {
	status = "disabled";
};

/* Remove a property, or a whole node. */
&led0 {
	/delete-property/ label;
};
/delete-node/ &led0;
```

The first block is [`button`](../examples/button/app.overlay)'s, the second is
[`blinky`](../examples/blinky/broken.overlay)'s deliberately broken overlay. Deleting `led0`
outright breaks the board's own `aliases` node, which still points at it:
`devicetree error: /aliases: undefined node label 'led0'`.

### Which overlays a build uses
The build looks for overlays in this order, and **uses the first that exists**:
1. The files given with **`-DDTC_OVERLAY_FILE=<files>`**. These *replace* everything below.
2. **`boards/<board>.overlay`** in the application, with the board target's slashes written as
   underscores: `boards/native_sim_native_64.overlay`.
3. **`<board>.overlay`** in the application directory itself.
4. **`app.overlay`**.

Files given with **`-DEXTRA_DTC_OVERLAY_FILE=<files>`** are then merged on top of whichever was
chosen, and that is the option to use for an extra overlay. The order is unlike Kconfig's, where a
board's `.conf` file is merged *with* `prj.conf`: an application with both `app.overlay` and
`boards/native_sim_native_64.overlay` gets only the second, and the nodes in `app.overlay` are
silently missing from the build. The build log says which files it took:

```text
-- Found BOARD.dts: .../deps/zephyr/boards/native/native_sim/native_sim_64.dts
-- Found devicetree overlay: .../lectures/L02/examples/button/app.overlay
```

### The result
The merged tree is written to **`build/zephyr/zephyr.dts`**, with every node and property annotated
with the file and line it came from. That is the file to read when a build does not do what the
overlay seemed to say:

```dts
aliases {
	led0 = &led0;        /* in deps/zephyr/boards/native/native_sim/native_sim.dts:35 */
	rtc = &rtc;          /* in deps/zephyr/boards/native/native_sim/native_sim.dts:36 */
	sw0 = &button0;      /* in lectures/L02/examples/button/app.overlay:20 */
};
```

Numbers come out in hex, and macros expanded: the button's flags are `0x11`, `GPIO_PULL_UP |
GPIO_ACTIVE_LOW`.

---

## Bindings
A devicetree on its own is only syntax: nothing in `gpios = <&gpio0 0 0>` says it is a GPIO
reference, or that an LED must have one. That is what a **binding** says. A binding is a YAML file
for one `compatible`, and this is Zephyr's for `gpio-leds`, from
`deps/zephyr/dts/bindings/led/gpio-leds.yaml`, without its examples:

```yaml
description: |
  Group of GPIO-controlled LEDs.

  Each LED is defined in a child node of the gpio-leds node.

compatible: "gpio-leds"

child-binding:
  description: GPIO LED child node

  include:
    - name: led-node.yaml
      property-allowlist:
        - label

  properties:
    gpios:
      type: phandle-array
      required: true
```

`child-binding` applies to the children of a `gpio-leds` node, which is why `led_0` needs no
`compatible` of its own. Every node in the tree is checked against its binding, and **macros are
generated only for the properties a binding declares**.

### Writing one
The [`bindings`](../examples/bindings) example has a binding of its own, in
`dts/bindings/qacademy,blink-pattern.yaml`:

```yaml
compatible: "qacademy,blink-pattern"

include: base.yaml

properties:
  pattern-name:
    type: string
    required: true

  led-gpios:
    type: phandle-array
    required: true

  delays-ms:
    type: array
    required: true

  repeat:
    type: int
    default: 1
```

| Key | Meaning |
|-----|---------|
| `compatible` | The nodes this binding is for. |
| `include` | Other bindings to inherit properties from. `base.yaml` brings the ones every node may have, such as `status`, `compatible` and `reg`. |
| `type` | `boolean`, `int`, `array` (of ints), `uint8-array`, `string`, `string-array`, `phandle`, `phandles` or `phandle-array`. |
| `required` | The build fails if a node lacks the property. |
| `default` | The value when a node does not set it. A property with a default always exists, as far as C can tell. |
| `enum` | The only values allowed. C gets the value's index in the list, too. |
| `child-binding` | A binding for the node's children. |

A property whose name ends in `-gpios` is by convention a phandle-array of GPIOs, and the GPIO
macros below take its name.

### Where bindings are found
The build searches `deps/zephyr/dts/bindings`, **the application's own `dts/bindings`**, and the
`dts/bindings` of every Zephyr module (L05). Nothing in `CMakeLists.txt` names the directory; it is
searched because it is there. The top of the generated header lists what was searched:

```text
 * Directories with bindings:
 *   .../lectures/L02/examples/bindings/dts/bindings, $ZEPHYR_BASE/dts/bindings
```

The part of a `compatible` before the comma is a **vendor prefix**, and the build checks it against
`vendor-prefixes.txt` files in the same directories. Without the example's own
`dts/bindings/vendor-prefixes.txt`, a plain build still succeeds, with a warning:

```text
node '/pattern-sos' compatible 'qacademy,blink-pattern' has unknown vendor prefix 'qacademy'
```

Twister, which L05 uses to build tests, treats devicetree warnings as errors, and fails.

### Nodes without a binding
A node whose `compatible` has no binding, or that has no `compatible` at all and no parent with a
`child-binding`, is still in the tree, and C can find out that it exists. But none of its
properties get macros. With `nobinding { count = <5>; };` in an overlay,
`DT_PROP(DT_PATH(nobinding), count)` does not compile:

```text
error: ‘DT_N_S_nobinding_P_count’ undeclared here (not in a function); did you mean ‘DT_N_S_nobinding_PARENT’?
```

There is one exception, made for exactly the case of an application that wants a few values of its
own without writing a binding: the node **`/zephyr,user`**. Its property types are inferred from
their values, so this works with no YAML at all:

```dts
/ {
	zephyr,user {
		blink-ms = <200 300>;
		led-gpios = <&gpio0 4 GPIO_ACTIVE_LOW>;
	};
};
```

### Errors a binding catches
A binding turns mistakes into errors **before a line of C is compiled**, and names the file and
line. These are all from Zephyr 4.4.2 builds.

A required property is missing:

```text
devicetree error: 'pattern-name' is marked as required in 'properties:' in '.../bindings/dts/bindings/qacademy,blink-pattern.yaml', but does not appear in <Node /pattern-heartbeat in .../bindings/app.overlay:18>
```

A value is not in the property's `enum`:

```text
devicetree error: value of property 'level' on /probe in .../zephyr/zephyr.dts.pre ('fatal') is not in 'enum' list in .../qacademy,probe.yaml (['info', 'warning', 'critical'])
```

A value has the wrong type. This one arrives at the end of a Python traceback, which is easy to
mistake for a crash of the build system:

```text
devicetree.dtlib.DTError: expected property 'limit' on /probe in .../type.overlay:1 to be assigned with 'limit = < (number) >;', not 'limit = "ten";'
```

And two that are not about bindings, but look the same:

```text
devicetree error: .../label.overlay:1 (column 1): parse error: undefined node label 'gpio9'
devicetree error: .../syntax.overlay:1 (column 59): parse error: expected ';' or ','
```

### A compatible also chooses a driver
For every compatible with an enabled node, the build defines a Kconfig symbol,
`DT_HAS_<COMPATIBLE>_ENABLED`, and drivers depend on it. `native_sim`'s `gpio0` is why
`build/zephyr/.config` has:

```text
CONFIG_DT_HAS_ZEPHYR_GPIO_EMUL_ENABLED=y
CONFIG_GPIO_EMUL=y
```

`CONFIG_GPIO_EMUL` has `default y` and `depends on DT_HAS_ZEPHYR_GPIO_EMUL_ENABLED`: once GPIO is
enabled, the driver is built because the devicetree has the part. It works in both directions. The
[`button`](../examples/button) example's `buttons` node is `compatible = "gpio-keys"`, which makes
`CONFIG_DT_HAS_GPIO_KEYS_ENABLED=y`, but the input subsystem is off, so no driver claims the
button, and the example handles the pin itself. Build it with `-DCONFIG_INPUT=y`, and the
`gpio-keys` input driver is enabled by the same rule, and refuses the node, which lacks the key
code that driver needs:

```text
error: static assertion failed: "zephyr-code must be specified to use the input-gpio-keys driver"
```

A `compatible` is not a comment. Choose it for the driver that should serve the node.

---

## From devicetree to C
### The generated header
The merged tree, checked against its bindings, is turned into
**`build/zephyr/include/generated/zephyr/devicetree_generated.h`**: some five thousand lines of
`#define`s for `native_sim`, included into every file through `<zephyr/devicetree.h>`. It is never read by hand in
full, but it is worth reading in part, because every devicetree macro in C expands into one of its
names.

Every node gets a **node identifier**, a C token made from its path: `DT_N`, then `_S_` for each
slash, with the node names lowercased and every character that is not a letter or digit turned
into `_`:

| Path | Node identifier |
|------|-----------------|
| `/` | `DT_N` |
| `/leds/led_0` | `DT_N_S_leds_S_led_0` |
| `/i2c@100` | `DT_N_S_i2c_100` |
| `/buttons/button_0` | `DT_N_S_buttons_S_button_0` |

Everything about a node is then a macro named after its identifier. For the button, among many:

```c
#define DT_N_S_buttons_S_button_0_P_gpios_IDX_0_PH DT_N_S_gpio_emul
#define DT_N_S_buttons_S_button_0_P_gpios_IDX_0_VAL_pin 1
#define DT_N_S_buttons_S_button_0_P_gpios_IDX_0_VAL_flags 17
#define DT_N_S_buttons_S_button_0_P_label "User button"
#define DT_N_S_buttons_S_button_0_STATUS_okay 1
#define DT_N_ALIAS_sw0         DT_N_S_buttons_S_button_0
#define DT_N_NODELABEL_button0 DT_N_S_buttons_S_button_0
```

### Getting a node identifier
Nobody writes `DT_N_S_buttons_S_button_0`. These macros build it:

| Macro | Finds the node by | Example |
|-------|-------------------|---------|
| `DT_ALIAS(alias)` | Alias. The portable way. | `DT_ALIAS(sw0)` |
| `DT_NODELABEL(label)` | Label. | `DT_NODELABEL(button0)` |
| `DT_PATH(...)` | Path, one argument per node. | `DT_PATH(buttons, button_0)` |
| `DT_CHOSEN(prop)` | A `/chosen` property. | `DT_CHOSEN(zephyr_console)` |
| `DT_INST(inst, compat)` | Instance number of a compatible. | `DT_INST(0, gpio_keys)` |

The arguments are written as C tokens: dashes and commas become underscores, so the alias `i2c-0`
is `DT_ALIAS(i2c_0)`, the chosen property `zephyr,console` is `DT_CHOSEN(zephyr_console)`, and the
compatible `qacademy,blink-pattern` is `qacademy_blink_pattern`. Most of these macros are one
token paste each: `DT_ALIAS(sw0)` is `DT_N_ALIAS_sw0`, which is `DT_N_S_buttons_S_button_0`.

**A node identifier is not a value.** It cannot be stored in a variable, compared at run time, or
chosen from a string: it is a token for the preprocessor to paste into other names. Everything in
this section is resolved at compile time, and costs nothing at run time; the price is that a
program cannot look up a node it did not name in its source.

### Properties
With a node identifier, the property macros read the tree:

| Macro | Result |
|-------|--------|
| `DT_PROP(node, prop)` | The value: an integer, a string literal, or for an array, a brace-enclosed initializer. |
| `DT_PROP_LEN(node, prop)` | The number of elements of an array, or of references in a phandle-array. |
| `DT_PROP_OR(node, prop, default)` | The value, or `default` if the node does not have the property. |
| `DT_NODE_HAS_PROP(node, prop)` | 1 if the node has the property, else 0. |
| `DT_ENUM_IDX(node, prop)` | The index of the value in the binding's `enum`. |
| `DT_NODE_EXISTS(node)` | 1 if the node exists, enabled or not. |
| `DT_NODE_HAS_STATUS_OKAY(node)` | 1 if the node exists and is enabled. |

Property names are written with underscores too: `delays-ms` is `DT_PROP(node, delays_ms)`. And
since an array property is an initializer, it can initialize an array:

```c
static const int blink_ms[] = DT_PROP(DT_PATH(zephyr_user), blink_ms);
```

A property that does not exist is a compile error naming the macro that is missing, which after
this section is readable. With `blinks` missing from `zephyr,user`:

```text
error: ‘DT_N_S_zephyr_user_P_blinks’ undeclared (first use in this function); did you mean ‘DT_N_S_zephyr_user_P_blink_ms’?
```

A missing alias fails the same way, one level earlier: `DT_ALIAS(led1)` expands to `DT_N_ALIAS_led1`,
which nothing defines, and every macro built on it fails with it in its name, such as
`‘DT_N_ALIAS_led1_P_gpios_IDX_0_VAL_pin’ undeclared`.

### Every node of a kind
`DT_FOREACH_STATUS_OKAY(compat, fn)` expands `fn(node_id)` once for every enabled node with a
compatible. The [`bindings`](../examples/bindings/src/main.c) example uses it to build a table at
compile time, so that adding a node to the overlay adds a table entry, with no change to the C:

```c
#define PATTERN_ENTRY(node_id)                                                                     \
	{                                                                                          \
		.name = DT_PROP(node_id, pattern_name),                                            \
		.led = GPIO_DT_SPEC_GET(node_id, led_gpios),                                       \
		.steps = DT_PROP_LEN(node_id, delays_ms),                                          \
		.repeat = DT_PROP(node_id, repeat),                                                \
	},

static const struct pattern patterns[] = {
	DT_FOREACH_STATUS_OKAY(qacademy_blink_pattern, PATTERN_ENTRY)};
```

The macro is itself only a name in the generated header, holding the list:

```c
#define DT_FOREACH_OKAY_qacademy_blink_pattern(fn) fn(DT_N_S_pattern_sos) fn(DT_N_S_pattern_heartbeat)
```

`fn` adds its own separators, here the trailing comma. Disabled nodes are left out, but they still
exist: `DT_NODE_EXISTS(DT_NODELABEL(strobe))` is 1.

Every node with a compatible also has an **instance number**, the `inst` of `DT_INST()`. Zephyr's
own documentation of `DT_INST()` says what they are and, more importantly, what they are not:
enabled nodes are numbered from 0, disabled nodes after them, and *"there is no general guarantee
that the same node will have the same instance number between builds"*. The order in which
`DT_FOREACH_STATUS_OKAY()` visits the nodes is not guaranteed either. A table built with it is in
the same order as the loop that reads it, which is all a program should rely on.

### Checking the tree at compile time
Because all of this is compile-time constants, the build can check the devicetree against what the
code assumes, with `BUILD_ASSERT()`:

```c
BUILD_ASSERT(DT_PROP(DT_NODELABEL(heartbeat), repeat) == 2, "heartbeat should play twice");
```

---

## Devices
### A device per node
A **device** is Zephyr's run-time object for a piece of hardware with a driver: a `struct device`,
with a name, an API, configuration, state, and an init function the kernel runs at boot. Drivers
define one for every enabled node of their compatible (L05 writes one), and C gets a pointer to it
from the node:

```c
const struct device *gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
```

`DEVICE_DT_GET()` is a compile-time address, not a search. The device's name, `gpio->name`, is the
node's `label` property if it has one, and otherwise its name: `gpio_emul`.

**A device can exist and still not be usable**, because its init function ran and failed: a chip
did not answer, a clock did not start. `device_is_ready()` returns true only for a device that
initialized successfully, and code that uses a device checks it first. The GPIO API has a shorthand
for a pin, `gpio_is_ready_dt(&spec)`.

### Ordinals, and the two errors
Nodes are numbered in **dependency order**: a node comes after every node it refers to, so that a
device is always initialized after the devices it depends on. The list is at the top of the
generated header:

```text
 * Node dependency ordering (ordinal and path):
 *   0   /
 ...
 *   22  /gpio_emul
 *   23  /buttons
 *   24  /buttons/button_0
```

and each node lists what it depends on:

```c
#define DT_N_S_buttons_S_button_0_REQUIRES_ORDS \
	22, /* /gpio_emul */ \
	23, /* /buttons */
```

The device object for a node is named after its ordinal: `DEVICE_DT_GET()` of `/gpio_emul` in this
build is `&__device_dts_ord_22`. **Ordinals belong to one build.** `/gpio_emul` is 22 in the
`button` example's build, 32 in `blinky`'s, and something else again once an overlay adds a node.
An ordinal in an error message means nothing until it is looked up in *that* build's header.

That name is what appears in the two errors a missing device causes, and the difference between
them says where to look. The [`blinky`](../examples/blinky/README.md) example's live demo produces
both:

```text
error: ‘__device_dts_ord_32’ undeclared here (not in a function); did you mean ‘__device_dts_ord_38’?
```

**Undeclared** is a compile error. A device object is declared only for an enabled node, so the node
exists but is disabled, or is not what the code thinks it is. Look at the devicetree. (The *did you
mean* is noise: 38 is only the nearest number that does exist.) CMake has usually warned already,
in a line easy to scroll past: `No SOURCES given to Zephyr library: drivers__gpio`.

```text
undefined reference to `__device_dts_ord_32'
```

**Undefined reference** is a link error. The node is enabled, so the object was declared, but no
driver defined it: the driver was not built. Look at Kconfig, here `CONFIG_GPIO=n`.

---

## GPIO
### A pin, from the devicetree
A GPIO pin is a controller device, a pin number and flags, and `struct gpio_dt_spec` holds exactly
that:

```c
struct gpio_dt_spec {
	const struct device *port;
	gpio_pin_t pin;
	gpio_dt_flags_t dt_flags;
};
```

`GPIO_DT_SPEC_GET(node, prop)` fills one from a phandle-array property, entirely at compile time:

```c
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
```

For the board's `led0`, that is `gpio_emul`'s device, pin 0, and flags 0. For a property with
another name, the name goes in: `GPIO_DT_SPEC_GET(node_id, led_gpios)`.

### Logical and physical
The GPIO functions ending in `_dt` take a `gpio_dt_spec`, and work in **logical** levels: 1 means
*active*, whatever that is on the pin. The flags from the devicetree decide what *active* is. For
an LED declared `GPIO_ACTIVE_LOW`, on `native_sim`:

| Call | Logical | Pin |
|------|---------|-----|
| `gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE)` | 0 | high |
| `gpio_pin_set_dt(&led, 1)` | 1 | low |
| `gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE)` | 1 | low |

That is how the C code stays the same when a board wires its LED the other way round: *on* is
always 1, and only the devicetree knows whether 1 is high. Functions with `_raw` in their name,
such as `gpio_pin_set_raw()`, bypass the flags and set the pin itself; use them only when the level
on the wire is what matters.

The flags given to `gpio_pin_configure_dt()` are combined with the flags from the devicetree, so
`GPIO_INPUT` on a button declared `GPIO_PULL_UP | GPIO_ACTIVE_LOW` configures an input with its
pull-up enabled.

### Interrupts and callbacks
A button is read by interrupt rather than by polling. Three calls set it up, as in the
[`button`](../examples/button/src/main.c) example:

```c
gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
gpio_init_callback(&button_cb, button_pressed, BIT(button.pin));
gpio_add_callback_dt(&button, &button_cb);
```

`GPIO_INT_EDGE_TO_ACTIVE` is logical too: an interrupt when the button becomes pressed, which is a
falling edge for an active-low button and a rising one for an active-high button.
`gpio_init_callback()` sets up a `struct gpio_callback` with the function to call and the pins it
is for, and `gpio_add_callback_dt()` adds it to the controller's list. The function receives the
controller, the callback, and a mask of the pins that fired:

```c
static void button_pressed(const struct device *port, struct gpio_callback *cb,
			   gpio_port_pins_t pins)
```

**On a microcontroller, the callback runs in the GPIO controller's interrupt service routine.** It
must not sleep, block or take long; L03 covers what it may do instead.

### The emulated controller
`native_sim` has no pins, so `gpio_emul` keeps them in memory, and has an API of its own, in
`<zephyr/drivers/gpio/gpio_emul.h>`, for a program or test to play the world outside the chip:

```c
int gpio_emul_input_set(const struct device *port, gpio_pin_t pin, int value);
int gpio_emul_output_get(const struct device *port, gpio_pin_t pin);
```

Both take and return **physical** levels, not logical ones. To press an active-low button, a
program drives its pin *low*; to find out whether an active-low LED is on, it checks for a *low*
output. Both have `_dt` variants taking a `gpio_dt_spec`, which are still physical. These functions
exist only on the emulator, and are the lines of the examples that would not build for a real
board.

Two of the emulator's behaviours match hardware, and one does not:
* **An interrupt needs an edge.** Setting an input to the level it already has fires nothing, and
  an input configured for `GPIO_INT_EDGE_TO_ACTIVE` fires nothing when it becomes inactive.
* **Pull resistors set the level.** Configuring an input with `GPIO_PULL_UP` sets its level high,
  and `GPIO_PULL_DOWN` low, as a real resistor would on an open pin.
* **The callback runs in the context that changed the pin.** `gpio_emul_input_set()` calls the
  callbacks itself, before it returns. Called from a thread, the callback runs in that thread, and
  `k_is_in_isr()` in it is false; called from a timer's expiry function, which runs in interrupt
  context, it runs in interrupt context. On a microcontroller it would always be an interrupt.
  Write callbacks as if they were: the emulator will not catch one that sleeps.

---

## Summary
* Devicetree describes the hardware, and C code refers to it by role. Zephyr turns the tree into C
  macros at build time, so there is nothing to parse at run time and nothing to change without a
  rebuild.
* A node has a path, labels, and possibly aliases pointing at it. `/aliases` gives applications
  portable names; `/chosen` tells Zephyr which nodes to use.
* Overlays add, change and delete nodes. A build uses one set of overlays, `DTC_OVERLAY_FILE`, a
  board overlay, or `app.overlay`, and merges `EXTRA_DTC_OVERLAY_FILE` on top.
  `build/zephyr/zephyr.dts` shows the result, and where each line came from.
* Bindings give nodes their meaning: property types, required properties, defaults and enums.
  Only declared properties reach C, except in `/zephyr,user`. A binding's errors come early and
  name the line.
* A `compatible` chooses a node's binding, and through `DT_HAS_<COMPAT>_ENABLED`, its driver.
* `DT_ALIAS()`, `DT_NODELABEL()` and friends give node identifiers, which are compile-time tokens.
  `DT_PROP()` and friends read properties, and `DT_FOREACH_STATUS_OKAY()` builds tables. Do not rely
  on instance numbers or on the order of the loop.
* `DEVICE_DT_GET()` gives a device, and `device_is_ready()` says whether it can be used.
  `__device_dts_ord_N` *undeclared* means a disabled node; *undefined reference* means a driver
  that was not built. Look N up in that build's `devicetree_generated.h`.
* GPIO through `gpio_dt_spec` is logical: 1 is active, and the devicetree says what active is.
  `gpio_emul`'s own functions are physical, and its callbacks run in the context that changed the
  pin.

---
