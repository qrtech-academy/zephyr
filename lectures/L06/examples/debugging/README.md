# Debugging
Correct code, and five ways to call it wrongly. Each bug is one wrong value passed to otherwise
correct code: an off-by-one size, a buffer freed too early, volts where millivolts were expected, a
thread that gives up, a value in the wrong unit. You pick the bug at build time with the Kconfig
choice in [`Kconfig`](./Kconfig), usually through one of the `.conf` files. On `native_sim` the
host's tools catch the first three: AddressSanitizer and UndefinedBehaviorSanitizer. The kernel's
fatal error handling catches the other two, through the handler in [`src/fatal.c`](./src/fatal.c).

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/debugging
west build -t run
```

Without a bug, every example runs correctly:

```text
[00:00:00.000,000] <inf> app: Filled 8 bytes
[00:00:00.000,000] <inf> app: Name starts with 's'
[00:00:00.000,000] <inf> app: 5 mV is 5000000 nV
[00:00:00.000,000] <inf> app: The configuration is valid
[00:00:00.000,000] <inf> app: Battery at 42 %
[00:00:00.000,000] <inf> app: main() is still running, and returns
```

---

## The bugs

### A stack buffer overflow, with AddressSanitizer

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/debugging -- -DEXTRA_CONF_FILE=asan.conf
west build -t run
```

```text
==1226562==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x742f412f4aa8 ...
WRITE of size 1 at 0x742f412f4aa8 thread T2
    #0 0x402bbe in fill .../lectures/L06/examples/debugging/src/main.c:28
...
    [32, 40) 'buf' (line 47) <== Memory access at offset 40 overflows this variable
```

AddressSanitizer stops the program at the first bad write, names the line, and names the variable it
overflowed. On a board, the same byte would silently overwrite whatever lies next to `buf`. The
threads are called `T1` and `T2` because every Zephyr thread on `native_sim` is a host pthread.

Add `-DCONFIG_APP_BUG_USE_AFTER_FREE=y` to the build command for the other ASan bug:
`ERROR: AddressSanitizer: heap-use-after-free`, with the line that read the memory, the line that
freed it, and the line that allocated it. GCC notices this one already at compile time:
`warning: pointer 'name' may be used after 'free' [-Wuse-after-free]`.

### A signed integer overflow, with UndefinedBehaviorSanitizer

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/debugging -- -DEXTRA_CONF_FILE=ubsan.conf
west build -t run
```

```text
.../src/main.c:35:20: runtime error: signed integer overflow: 5000 * 1000000 cannot be represented in type 'int'
[00:00:00.000,000] <inf> app: 5000 mV is 705032704 nV
```

UBSan reports the overflow and lets the program continue with the wrapped result. Without it, the
program prints the same `705032704` and nothing else.

### A kernel oops, caught by the fatal error handler

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/debugging -- -DEXTRA_CONF_FILE=oops.conf
west build -t run
```

```text
[00:00:00.000,000] <err> app: The configuration is broken
[00:00:00.000,000] <err> os: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
[00:00:00.000,000] <err> os: Current thread: 0x410720 (checker)
[00:00:00.000,000] <err> fatal: Caught fatal error 3 in thread 'checker'
[00:00:00.000,000] <err> fatal: An oops: aborting only that thread
[00:00:00.000,000] <inf> app: Battery at 42 %
[00:00:00.000,000] <inf> app: main() is still running, and returns
```

`k_sys_fatal_error_handler()` returns for an oops, so the kernel aborts the `checker` thread and
nothing else. `k_thread_join()` in `main()` returns, because an aborted thread counts as finished,
and the program carries on.

### A failed assertion

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/debugging -- -DEXTRA_CONF_FILE=assert.conf
west build -t run
```

```text
ASSERTION FAIL [percent <= 100] @ WEST_TOPDIR/lectures/L06/examples/debugging/src/main.c:41
	a battery cannot be 420 % full
[00:00:00.000,000] <err> os: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
[00:00:00.000,000] <err> os: Current thread: 0x413f60 (main)
[00:00:00.000,000] <err> fatal: Caught fatal error 4 in thread 'main'
[00:00:00.000,000] <err> fatal: Halting the system
Exiting due to fatal error
```

A failed assertion is a kernel panic. The handler must not return from one, so it halts, and on
`native_sim` halting means the process exits with status 1.

---

## What to look at
* **`__ASSERT()` is compiled out unless `CONFIG_ASSERT=y`.** Build with only
  `-DCONFIG_APP_BUG_ASSERT=y` and the program prints `Battery at 420 %` without complaint. With it,
  CMake warns `__ASSERT() statements are globally ENABLED`, because assertions cost flash and time.
* **AddressSanitizer only exists on `native_sim`.** `CONFIG_ASAN` depends on `ARCH_POSIX`: it
  instruments the program for the host's runtime library. UndefinedBehaviorSanitizer's reports
  need a runtime too, the host's or picolibc's; on other boards `CONFIG_UBSAN_TRAP` only turns
  undefined behaviour into a fault, without saying what or where. That is a large part of why it
  pays to keep code runnable on `native_sim` even when the product is a board.
* **`LOG_PANIC()`** at the top of the handler switches logging to synchronous. On a board, logging
  is deferred by default: messages go to a thread of its own, which may never run again after a
  fatal error. `native_sim` logs in immediate mode already, so here it changes nothing you can see.
* **[`sample.yaml`](./sample.yaml)** checks the correct run and the oops. It only builds the other
  three: a program that exits with status 1 fails in Twister whatever it printed, and UBSan reports
  on stderr, which Twister's console harness does not read.

---

## Debugging with GDB
`zephyr.exe` is a Linux program, so GDB debugs it directly, without a probe or a GDB server.
[`prj.conf`](./prj.conf) sets `CONFIG_DEBUG_OPTIMIZATIONS=y`, which compiles with `-Og`, so variables
and lines match the source.

```bash
sudo apt install gdb
gdb --args ./build/zephyr/zephyr.exe -no-rt
(gdb) break fill
(gdb) break k_sys_fatal_error_handler
(gdb) run
(gdb) backtrace
(gdb) print size
(gdb) info threads
```

`info threads` lists host threads: native_sim creates one for every Zephyr thread when the Zephyr
thread is created, next to the simulator's own.
`CONFIG_ARCH_POSIX_TRAP_ON_FATAL=y` raises `SIGTRAP` on a fatal error before the process exits, so
GDB stops at the error even without a breakpoint.

---

## Coverage
With `CONFIG_COVERAGE=y`, the program writes gcov data (`.gcda` files, next to the object files in the
build directory) when it exits. Twister runs the tests and turns that data into an HTML report:

```bash
sudo apt install lcov
west twister -T lectures/L06/examples/debugging -p native_sim/native/64 --coverage --coverage-tool lcov
```

The report is `twister-out/coverage/index.html`. Twister's default tool is gcovr
(`pip install gcovr`), which needs no `--coverage-tool`. For a single build, `gcov` reads the data
directly:

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/debugging -- -DCONFIG_COVERAGE=y
./build/zephyr/zephyr.exe -no-rt -stop_at=1
gcov -o build/CMakeFiles/app.dir/src/main.c.gcda lectures/L06/examples/debugging/src/main.c
```

`gcov` prints `Lines executed:100.00% of 53` for the correct run, and writes `main.c.gcov` with a run
count beside every line.

---
