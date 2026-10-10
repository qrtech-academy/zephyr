# Settings
A boot counter that survives a restart. The settings subsystem keeps key-value pairs in the
`storage_partition` of the board's flash, through ZMS (Zephyr Memory Storage). On `native_sim` that
flash is simulated in a file, so the counter survives exactly as long as the file does.

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/settings
west build -t run
```

Stop it with `Ctrl+C`, run it again, and the count has gone up:

```text
[00:00:00.000,000] <inf> app: Boot count: 1
...
[00:00:00.000,000] <inf> app: Boot count: 2
```

---

## What to look at
* **The flash is a file, `flash.bin`, in the directory the program runs in.** `west build -t run`
  runs it in the build directory, so the file is `build/flash.bin`, and `west build -p` deletes it
  along with everything else: a pristine build is a factory-new device. Running
  `./build/zephyr/zephyr.exe` from the repository root makes a different `flash.bin`, there.
* **The command-line options choose what happens to the flash** (`./build/zephyr/zephyr.exe --help`
  lists them): `-flash=<path>` uses another file, `-flash_erase` erases it at startup, `-flash_rm`
  deletes the file when the program ends, and `-flash_in_ram` keeps the flash in RAM only, so every
  run is a first boot.
* **`strings build/flash.bin`** prints `boot/count`: the setting's name is stored next to its value.
  The name is how `settings_load()` finds the handler for the value. The handler registered for
  the subtree `boot` gets called with the rest of the name, `count`.
* **The handler reads the value itself**, through `read_cb`, because the settings subsystem does not
  know the value's type. It checks the stored length first. A stored value that is not the size of
  `boot_count` is rejected rather than copied half-way.
* **ZMS logs its mount** (`fs_zms: 4 Sectors of 4096 bytes`) at level `INF`.
  `CONFIG_ZMS_LOG_LEVEL_WRN=y` in [`prj.conf`](./prj.conf) hides it. Remove that line to see how the
  16 KiB storage partition is laid out.

---
