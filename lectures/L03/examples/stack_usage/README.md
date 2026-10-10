# Stack Usage
A thread with a 1024-byte stack calls a function with a 2048-byte local buffer. On a
microcontroller that overflows the stack; on `native_sim` it runs without complaint, because a
`native_sim` thread does not really run on the stack Zephyr gives it. What does find the problem
without hardware is the compiler: `CONFIG_STACK_USAGE=y` builds with `-fstack-usage`, which writes
each function's stack frame size into a `.su` file beside its object file.

```bash
west build -p -b native_sim/native/64 lectures/L03/examples/stack_usage
./build/zephyr/zephyr.exe -stop_at=1 -no-rt
```

```text
round 1: report of 2048 bytes, on a stack of 1024 bytes
round 2: report of 2048 bytes, on a stack of 1024 bytes
round 3: report of 2048 bytes, on a stack of 1024 bytes
```

The application's own functions are in `build/CMakeFiles/app.dir/src/main.c.su`, one per line: the
source location and name, the frame size in bytes, and whether the size is fixed (`static`) or
depends on something like a variable-length array (`dynamic`):

```bash
cat build/CMakeFiles/app.dir/src/main.c.su
```

```text
.../src/main.c:18:13:print_report	2096	dynamic,bounded
.../src/main.c:27:13:reporter_entry	16	static
.../src/main.c:42:5:main	8	static
```

And the functions with the largest frames anywhere in the build, Zephyr's included:

```bash
find build -name '*.su' -exec cat {} + | sort -t$'\t' -k2 -n | tail
```

---

## What to look at
* **`print_report` needs 2096 bytes, on a thread with 1024.** Nothing at run time says so. The
  stack size in `K_THREAD_DEFINE` only matters on a real target.
* **Why nothing at run time says so.** A `native_sim` thread runs on a host (pthread) stack, and
  keeps only a little bookkeeping in the stack Zephyr allocated for it. So Zephyr's runtime stack
  checks cannot work here: `CONFIG_THREAD_ANALYZER` depends on `!ARCH_POSIX`, and asking for it
  aborts the build, while stack canaries and hardware stack protection are listed as unsupported in
  `deps/zephyr/boards/native/doc/arch_soc.rst`. On a board, the thread analyzer's high-water marks
  are the tool to use.
* **A `.su` number is one function's frame, not a thread's need.** A thread needs the sum of the
  frames along its deepest call chain: here `reporter_entry`, `print_report`, and `snprintf()` and
  `printk()` below it. `snprintf()` comes from the host's C library on `native_sim`, so it has no
  `.su` file at all.
* **The numbers depend on the architecture and the compiler options.** These are x86-64 frames;
  the method carries over to a target, the numbers do not.

---
