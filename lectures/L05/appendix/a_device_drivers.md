# Appendix A
## Device Drivers
This appendix covers how Zephyr turns a devicetree node into a device, what a driver provides for
it, and how a driver for a new part is packaged: binding, Kconfig option, and module. By the end you
should be able to explain every file in [`examples/thermo/module`](../examples/thermo/module), and
why each one is needed.

---

## What a driver is, in Zephyr
L02 used devices without asking where they came from: `DEVICE_DT_GET(DT_ALIAS(led0))` returned a
`const struct device *`, and the GPIO API did the rest. This lecture is the other side of that call.

A Zephyr driver is three things:
1. **A set of functions** that implement one of Zephyr's device APIs, such as GPIO, I2C or sensor,
   for one kind of part.
2. **An init function**, which the kernel calls once at boot to bring an instance of the part up.
3. **A macro expansion per devicetree node**, which creates a `struct device` for every node that
   the driver serves, at compile time, with the node's properties already filled in.

There is no registration at run time and no probing. By the time the program links, every device
exists as a constant in flash, and the only thing left to happen at boot is the init function.

---

## struct device
Every device is a `struct device`, defined in `<zephyr/device.h>`. Its first five members are the
ones a driver writer deals with:

```c
struct device {
	const char *name;           /* The devicetree node's name, e.g. "thermo@48". */
	const void *config;         /* Per-instance config: const, from devicetree. */
	const void *api;            /* The function table: the driver's API struct. */
	struct device_state *state; /* Owned by the kernel: init's state and result. */
	void *data;                 /* Per-instance run-time data: mutable. */
	/* ... */
};
```

**`config` and `data` are the driver's own types**, which the kernel stores as `void *`. The
difference between them matters on a microcontroller:
* **`config` is `const`.** It holds what the devicetree says about the instance: its bus, its
  address, its properties. It is built at compile time and lives in flash.
* **`data` is not.** It holds what changes at run time: the last reading, a mutex, a state machine.
  It lives in RAM.

In [`thermo.c`](../examples/thermo/module/drivers/sensor/thermo/thermo.c) they are:

```c
struct thermo_config {
	struct i2c_dt_spec i2c;
	uint32_t conversion_timeout_ms;
};

struct thermo_data {
	int16_t raw;
};
```

and every driver function starts by getting them back out of the device:

```c
const struct thermo_config *config = dev->config;
struct thermo_data *data = dev->data;
```

**The name is the node's name**, `thermo@48`, not its label. That is what `dev->name` prints and what
the logs show.

---

## The API struct
Each kind of device has an API: a struct of function pointers declared by its subsystem. For the
sensor subsystem it is `struct sensor_driver_api` in `<zephyr/drivers/sensor.h>`, whose two
mandatory members are:

```c
sensor_sample_fetch_t sample_fetch;
sensor_channel_get_t channel_get;
```

The functions an application calls, such as `sensor_sample_fetch(dev)`, are thin wrappers that cast
`dev->api` and call through it:

```c
static inline int z_impl_sensor_sample_fetch(const struct device *dev)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	return api->sample_fetch(dev, SENSOR_CHAN_ALL);
}
```

A driver fills in its own table with `DEVICE_API()`:

```c
static DEVICE_API(sensor, thermo_api) = {
	.sample_fetch = thermo_sample_fetch,
	.channel_get = thermo_channel_get,
};
```

`DEVICE_API(sensor, name)` declares a `const struct sensor_driver_api`, and also places it in a
linker section of its own, one per API class. That lets `DEVICE_API_IS(sensor, dev)` check at run
time whether a device really has a sensor API, rather than trusting whoever passed the pointer.

This is Zephyr's polymorphism: the application's `sensor_sample_fetch()` works the same for Thermo,
for a Bosch accelerometer, or for any of the hundreds of sensor drivers in the tree, and none of
them knows about the others.

---

## From devicetree node to device
### DT_DRV_COMPAT and instances
A driver serves every node with a given `compatible`, and says which one at the top of its file:

```c
#define DT_DRV_COMPAT qacademy_thermo
```

The value is the compatible `"qacademy,thermo"` in lowercase, with every character that cannot be in
a C identifier turned into an underscore. From then on, the `DT_INST_...` macros count the
**instances** of that compatible whose `status` is `okay`, numbered 0, 1, 2, ... in an order the
build decides. `DT_INST_PROP(inst, conversion_timeout_ms)` is `DT_PROP()` for instance `inst`.

### One device per instance
The driver writes a macro that defines everything for one instance, and expands it for all of them:

```c
#define THERMO_DEFINE(inst)                                                    \
	static struct thermo_data thermo_data_##inst;                          \
                                                                               \
	static const struct thermo_config thermo_config_##inst = {             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                             \
		.conversion_timeout_ms = DT_INST_PROP(inst, conversion_timeout_ms), \
	};                                                                     \
                                                                               \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, thermo_init, NULL, &thermo_data_##inst, \
				     &thermo_config_##inst, POST_KERNEL,       \
				     CONFIG_SENSOR_INIT_PRIORITY, &thermo_api);

DT_INST_FOREACH_STATUS_OKAY(THERMO_DEFINE)
```

With two Thermo nodes in the devicetree, that is two `thermo_data`, two `thermo_config`, and two
devices; with none, it is nothing at all. `I2C_DT_SPEC_INST_GET(inst)` fills in the bus device and
the address from the node's parent and its `reg` property.

The device macro takes, in order:

| Argument | Meaning |
|----------|---------|
| `inst` | The instance. The macro turns it into the node, and the node into the device's name and symbol. |
| `thermo_init` | The init function. |
| `NULL` | Power management actions, which Thermo has none of. |
| `&thermo_data_##inst` | `dev->data`. May be `NULL`. |
| `&thermo_config_##inst` | `dev->config`. |
| `POST_KERNEL` | The init level. |
| `CONFIG_SENSOR_INIT_PRIORITY` | The priority within the level. |
| `&thermo_api` | `dev->api`. |

`SENSOR_DEVICE_DT_INST_DEFINE()` is `DEVICE_DT_INST_DEFINE()` plus the sensor subsystem's
bookkeeping; other subsystems' drivers use `DEVICE_DT_INST_DEFINE()` directly.

### The symbol behind DEVICE_DT_GET
A device is a global variable named after the node's position in the devicetree's dependency
order: `__device_dts_ord_32` is the device of the node with ordinal 32, and the ordinals are listed
at the top of `build/zephyr/include/generated/zephyr/devicetree_generated.h`. `DEVICE_DT_GET(node)`
is a reference to that variable.

That explains the error L02 met, from the other side. `DEVICE_DT_GET()` of an enabled node that no
driver defined a device for compiles, since the name is only a symbol, but the link fails:

```text
undefined reference to `__device_dts_ord_32'
```

The node exists; nothing defined its device. The causes are a driver that is not enabled, a driver
whose `DT_DRV_COMPAT` does not match the node, or a module that is not in the build. In the Thermo
example, removing the module from `CMakeLists.txt` does exactly that.

---

## Initialization
### Levels and priorities
The kernel initializes devices in levels, in this order:

| Level | When |
|-------|------|
| `EARLY` | Right after entering C, before the kernel's architecture-specific initialization. |
| `PRE_KERNEL_1`, `PRE_KERNEL_2` | Before the kernel's services are available: no sleeping, no kernel objects. Interrupt controllers, clocks, the system timer, consoles. |
| `POST_KERNEL` | After the kernel is running. Most device drivers, since they may need to sleep or take a mutex. |
| `APPLICATION` | Just before `main()`. |

Within a level, devices run in order of their **priority**, a number from 0 to 999, lowest first.
Subsystems give their drivers a Kconfig option for it: I2C controllers use
`CONFIG_I2C_INIT_PRIORITY`, which defaults to `CONFIG_KERNEL_INIT_PRIORITY_DEVICE` (50), and
sensors use `CONFIG_SENSOR_INIT_PRIORITY` (90). A sensor on an I2C bus therefore initializes after
the bus, which it needs.

**The build checks that.** `CONFIG_CHECK_INIT_PRIORITIES`, on by default, compares the init order
in the image with the dependencies in the devicetree, and fails the build if a device initializes
before a device its node depends on, such as a sensor before its bus.

To see the order, list the build's init entries:

```bash
west build -t initlevels
```

For the L05 exercises' test application with two Lumo parts, the end of the list is:

```text
POST_KERNEL
  __init_enable_logger: enable_logger(NULL)
  __init___device_dts_ord_31: i2c_emul_init(__device_dts_ord_31)
  __init___device_dts_ord_32: lumo_init(__device_dts_ord_32)
  __init___device_dts_ord_33: lumo_init(__device_dts_ord_33)
```

The list is read from the image, so it also shows a device whose initialization is deferred (below),
which the kernel then skips at boot.

### The init function
An init function takes the device and returns 0 or a negative error number:

```c
static int thermo_init(const struct device *dev)
{
	const struct thermo_config *config = dev->config;
	uint8_t id;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, THERMO_REG_WHO_AM_I, &id);
	if (ret < 0) {
		return ret;
	}

	if (id != THERMO_WHO_AM_I_VALUE) {
		return -ENODEV;
	}

	return 0;
}
```

What belongs in it: checking that the bus is ready, that the part answers, and that it is the part
the devicetree says it is; and putting the part into the state the devicetree asks for. What does
not: anything slow that a driver could do on first use instead. Every init function runs before
`main()`, one after another.

### device_is_ready
The kernel records whether a device's init function ran, and what it returned.
`device_is_ready(dev)` is true only if both went well:

```c
return dev->state->initialized && (dev->state->init_res == 0U);
```

**A failed init is final.** Nothing retries it, and every call into the device's API would talk to a
part that is not there or not right. That is why every user of a device checks `device_is_ready()`
once, before using it, and why a driver's init must fail when the part is wrong rather than log a
warning and carry on.

### Deferred initialization
A node with the property `zephyr,deferred-init` is not initialized at boot. The application
initializes it later, when it chooses, with `device_init(dev)`: for example once a power rail has
been switched on. `device_init()` returns what the init function returned, or `-EALREADY` if the
device was initialized before, **whether or not that went well**. The Thermo tests use exactly this
to test a failing init: they break the emulated part, call `device_init()`, and get `-ENODEV`; a
second call, after repairing the part, gets `-EALREADY`, and the device stays not ready.

---

## A binding and a Kconfig option for a new part
### The binding
L02 wrote a binding for a node of the application's own. A driver's binding is the same kind of
file, and says what a node for the part must look like:

```yaml
description: |
  Thermo, a made-up I2C temperature sensor ...

compatible: "qacademy,thermo"

include: [sensor-device.yaml, i2c-device.yaml]

properties:
  conversion-timeout-ms:
    type: int
    default: 100
```

`i2c-device.yaml` is Zephyr's common binding for a part on an I2C bus: it requires `reg`, the
address, and makes the node valid only as a child of an I2C controller. `sensor-device.yaml` adds
the properties every sensor may have. A property with an `enum:` list may only take one of the
listed values, and `DT_INST_ENUM_IDX(inst, prop)` gives the index of the value the node chose, at
compile time.

### The vendor prefix
The part before the comma in a compatible is a **vendor prefix**, and Zephyr keeps a list of known
ones in `dts/bindings/vendor-prefixes.txt`. A module can add its own, in a file of the same name:

```text
qacademy	QRTECH Academy
```

Without it, a normal build only warns, once per node:

```text
node '/i2c@100/lumo@29' compatible 'qacademy,lumo' has unknown vendor prefix 'qacademy'
```

**Twister turns that warning into an error.** It configures every build with
`-DEXTRA_GEN_EDT_ARGS=--edtlib-Werror`, and the same message becomes a `devicetree error:` that
stops the configuration. A module that builds fine by hand and fails under Twister has usually
missed exactly this file.

### The Kconfig option
Every driver has a Kconfig option that decides whether it is compiled:

```text
config THERMO
	bool "Thermo I2C temperature sensor"
	default y
	depends on SENSOR
	depends on DT_HAS_QACADEMY_THERMO_ENABLED
	select I2C
```

**`DT_HAS_QACADEMY_THERMO_ENABLED` is generated from the devicetree.** For every compatible that has
a binding, the build writes a symbol into `build/Kconfig/Kconfig.dts` that is `y` when at least one
node with that compatible is enabled:

```text
DT_COMPAT_QACADEMY_LUMO := qacademy,lumo

config DT_HAS_QACADEMY_LUMO_ENABLED
	def_bool $(dt_compat_enabled,$(DT_COMPAT_QACADEMY_LUMO))
```

With `default y`, the driver is on exactly when the hardware is there, and nobody sets `CONFIG_THERMO`
in a `prj.conf`. This is the pattern of nearly every driver in Zephyr, and the reason enabling a
driver usually means adding a devicetree node rather than setting an option. The remaining
dependencies are the subsystem the driver plugs into (`SENSOR`, which the application enables) and
the bus it needs (`I2C`, which the driver selects).

One consequence is easy to miss. A `default` that does not take is not an assignment, so nothing
warns about it: an application with a Thermo node but without `CONFIG_SENSOR=y` builds its
devicetree, leaves `THERMO` off, and fails at link time with `undefined reference to
'__device_dts_ord_32'`.

---

## The sensor API
### Fetch and get
Reading a sensor takes two calls:

```c
ret = sensor_sample_fetch(dev);
ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &value);
```

**`sample_fetch` talks to the part**: it starts a measurement if the part needs one, reads the raw
result, and stores it in `dev->data`. It can fail, and it can take time. `sensor_sample_fetch(dev)`
fetches every channel the part has (`SENSOR_CHAN_ALL`), and `sensor_sample_fetch_chan(dev, chan)`
only one.

**`channel_get` only converts** the stored result into the channel's unit. It does no I/O, and it
returns the same value every time until the next fetch. The split lets a part that measures several
things at once, such as an accelerometer's three axes, be read in one bus transaction and then
converted channel by channel. Both return `-ENOTSUP` for a channel the part does not have.

### struct sensor_value
A reading is a fixed-point number in two parts:

```c
struct sensor_value {
	int32_t val1;   /* Integer part. */
	int32_t val2;   /* Fractional part, in millionths. */
};
```

Each channel has a unit: degrees Celsius for `SENSOR_CHAN_AMBIENT_TEMP`, lux for
`SENSOR_CHAN_LIGHT`. **Both parts carry the sign**: -1.5 degrees is `val1 = -1, val2 = -500000`, and
-0.5 degrees is `val1 = 0, val2 = -500000`. A driver should not assemble the two parts by hand;
`sensor_value_from_micro(&value, micro)` does it from a value in millionths, and
`sensor_value_to_micro(&value)` converts back. Work in `int64_t` millionths: a 16-bit count times
250000 millionths per count does not fit in 32 bits.

### The newer, asynchronous API
Zephyr also has an asynchronous sensor API (`CONFIG_SENSOR_ASYNC_API`), built on RTIO, in which a
driver provides `submit` and `get_decoder` instead, and readings are decoded into Q31 fixed-point
values. It suits high-rate and streaming sensors. The fetch and get API is what most existing
drivers implement, and what this course uses; Q31 values reappear in Appendix B, in the emulator
backend.

---

## Zephyr modules
### Out of tree
A driver for a part that Zephyr does not support does not have to live in Zephyr. A **module** is a
directory that the build treats as an extension of Zephyr: its CMake is included into Zephyr's
build, its Kconfig into Zephyr's configuration, and its bindings into the devicetree search path.
Vendor HALs, mbedTLS and LVGL are all modules; so is `examples/thermo/module`.

What makes a directory a module is one file, `zephyr/module.yml`:

```yaml
name: qacademy-thermo
build:
  cmake: .
  kconfig: Kconfig
  settings:
    dts_root: .
```

| Key | Meaning |
|-----|---------|
| `name` | The module's name. The build also defines a Kconfig symbol for it: `CONFIG_ZEPHYR_QACADEMY_THERMO_MODULE=y`. |
| `build: cmake` | The directory whose `CMakeLists.txt` the build includes. |
| `build: kconfig` | The Kconfig file the build sources. |
| `settings: dts_root` | A directory whose `dts/bindings/` holds bindings, and whose `dts/bindings/vendor-prefixes.txt` holds vendor prefixes. |

The module's `CMakeLists.txt` is included after Kconfig has run, so it compiles the driver only when
the driver is enabled:

```cmake
add_subdirectory_ifdef(CONFIG_THERMO drivers/sensor/thermo)
```

and the driver's own `CMakeLists.txt` makes it a library of the Zephyr image, as in-tree drivers do:

```cmake
zephyr_library()
zephyr_library_sources(thermo.c)
```

### Adding a module to a build
In a west workspace, the modules are the projects of the manifest, as Appendix B of L01 described.
A module that is not a west project is added by path, in `ZEPHYR_EXTRA_MODULES`, before
`find_package(Zephyr)`:

```cmake
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../module)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
```

or on the command line, as `-DZEPHYR_EXTRA_MODULES=<path>`. The build merges the two, so an
application can name the modules it always needs in its `CMakeLists.txt`, and a test can add
another, such as an emulator, from outside.

---

## Summary
* A driver is an API implementation, an init function, and a macro expanded once for every enabled
  devicetree node of its compatible.
* A device's `config` is `const`, from devicetree, in flash; its `data` changes at run time, in RAM.
* `DT_DRV_COMPAT` names the compatible; `DT_INST_FOREACH_STATUS_OKAY()` and
  `DEVICE_DT_INST_DEFINE()` make one device per instance.
* `undefined reference to '__device_dts_ord_N'` means a node that no driver made a device for.
* Devices initialize by level, then by priority. `west build -t initlevels` shows the order, and
  the build checks it against the devicetree.
* A failed init is final: `device_is_ready()` is false from then on. `zephyr,deferred-init` and
  `device_init()` move init to a moment the application chooses.
* A new part needs a binding, a vendor prefix, and a Kconfig option that depends on
  `DT_HAS_<COMPAT>_ENABLED`, so that the devicetree node alone enables the driver.
* The sensor API splits I/O (`sample_fetch`) from conversion (`channel_get`), and returns fixed-point
  `sensor_value`s whose two parts both carry the sign.
* A module packages drivers, bindings and Kconfig outside Zephyr, and is added to a build with
  `ZEPHYR_EXTRA_MODULES` or a west manifest.

---
