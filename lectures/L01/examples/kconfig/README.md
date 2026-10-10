# Kconfig
An application with Kconfig options of its own, set from `prj.conf`, from an extra configuration
file, and read from both C and CMake.

```bash
west build -p -b native_sim/native/64 lectures/L01/examples/kconfig
west build -t run
```

With the extra configuration file merged on top of `prj.conf`:

```bash
west build -p -b native_sim/native/64 lectures/L01/examples/kconfig -- -DEXTRA_CONF_FILE=shout.conf
west build -t run
```

---

## What to look at
* **[`Kconfig`](./Kconfig)** declares the options and ends with `source "Kconfig.zephyr"`. Remove
  that line and the build fails, because the application's Kconfig replaces Zephyr's top level.
* **`west build -t menuconfig`** shows the options under *Application*. Change one, save, and
  rebuild: the change lives in `build/zephyr/.config` only, and `west build -p` throws it away.
* **[`CMakeLists.txt`](./CMakeLists.txt)** compiles `src/shout.c` only when `CONFIG_APP_SHOUT=y`.
* **`CONFIG_APP_REPEAT=20`** in `prj.conf` is outside the option's `range`. Zephyr treats the
  warning as an error and aborts the build; the message says which value it wanted instead.
* **`IS_ENABLED(CONFIG_APP_SHOUTT)`**, misspelled, is not an error at all: an undefined macro is
  simply off. That is the Kconfig mistake that survives to run time.

---
