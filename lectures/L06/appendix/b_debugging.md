# Appendix B
## Debugging: Fatal Errors, Sanitizers, GDB and Coverage
This appendix covers what happens when a Zephyr program goes wrong, and the tools that find out
why. Some of them are Zephyr's own and work the same on a board: fatal errors, the handler that
decides what one costs, and assertions. The others are the host's, and are much of the reason to
keep code runnable on `native_sim`: AddressSanitizer, UndefinedBehaviorSanitizer, GDB and gcov. By
the end you should be able to explain every line the [`debugging`](../examples/debugging) example
prints, in each of its configurations.

---

## Fatal errors
### What makes an error fatal
A **fatal error** is one the running code cannot continue from. The kernel names the reason with a
number:

| Reason | Value | Raised by |
|--------|-------|-----------|
| `K_ERR_CPU_EXCEPTION` | 0 | The CPU: a bus fault, an illegal instruction, a division by zero, on a board. |
| `K_ERR_SPURIOUS_IRQ` | 1 | An interrupt that has no handler. |
| `K_ERR_STACK_CHK_FAIL` | 2 | A stack overflow check. |
| `K_ERR_KERNEL_OOPS` | 3 | `k_oops()`: *this thread* cannot go on. |
| `K_ERR_KERNEL_PANIC` | 4 | `k_panic()`, and every failed assertion: *the system* cannot go on. |

On `native_sim` there is no CPU exception to raise, since the code runs natively; a genuine
segmentation fault kills the host process like any other Linux program's. Oopses, panics and
assertions work exactly as on a board.

**`k_oops()` and `k_panic()` are for code that has found something impossible**, not for ordinary
errors. A sensor that does not answer is an error to return; a reading below absolute zero, from a
sensor the code has already validated, is a broken assumption, and an oops is how a thread gives
up on it.

### What the kernel does
Every fatal error goes through the same path. The kernel logs what happened, and then calls
**`k_sys_fatal_error_handler()`** with the reason:

```text
[00:00:00.000,000] <err> app: Impossible reading: -300 C
@ WEST_TOPDIR/lectures/L06/exercises/exercise3/src/main.c:29
[00:00:00.000,000] <err> os: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
[00:00:00.000,000] <err> os: Current thread: 0x40f6c0 (sampler)
```

* **`@ file:line`** is printed on architectures that have no CPU exception to raise for an oops,
  `native_sim` among them. On a board, the exception's registers say where instead.
* **`ZEPHYR FATAL ERROR 3: Kernel oops`** gives the reason's number and name. Twister looks for this
  exact string to tell a crashed test from a failed one.
* **`Current thread`** names the thread, if `CONFIG_THREAD_NAME=y`; otherwise only its address.

All of it is logged, so **without `CONFIG_LOG=y` a fatal error prints nothing** but what the handler
prints itself.

### The handler
The kernel's own `k_sys_fatal_error_handler()` is a weak function that halts the system, whatever the
reason:

```text
[00:00:00.000,000] <err> os: Halting system
Exiting due to fatal error
```

On a board, halting means an endless loop with interrupts off, until the watchdog or a person resets
the device. On `native_sim`, it means that the process exits with **status 1**, after printing
`Exiting due to fatal error` on stderr.

An application that wants a different policy defines the function itself, which replaces the weak
one:

```c
#include <zephyr/fatal.h>
#include <zephyr/logging/log_ctrl.h>

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	LOG_PANIC();
	LOG_ERR("Fatal error %u in thread '%s'", reason, k_thread_name_get(k_current_get()));

	if (reason == K_ERR_KERNEL_OOPS) {
		return;
	}
	k_fatal_halt(reason);
}
```

**What the handler decides is what the error costs:**
* **If it returns**, the kernel aborts only the thread that caused the error, and every other thread
  keeps running. A thread waiting in `k_thread_join()` for the aborted one returns, because an
  aborted thread counts as finished.
* **If it calls `k_fatal_halt()`**, the system halts, as with the default handler.

**Returning from a panic is not allowed.** With `CONFIG_ASSERT=y`, the kernel checks for it, and the
check is itself an assertion, so it panics again, and calls the same handler, which returns again.
On `native_sim` that recursion ends when the host process runs out of stack, with a segmentation
fault and exit status 139:

```text
ASSERTION FAIL [reason != K_ERR_KERNEL_PANIC] @ WEST_TOPDIR/deps/zephyr/kernel/fatal.c:135
	Attempted to recover from a kernel panic condition
```

`LOG_PANIC()` switches logging to synchronous. On a board, logging is deferred by default (L01): the
messages wait for a thread of their own, which may never run again after a fatal error. On
`native_sim` logging is immediate already, so the call changes nothing you can see there; it
belongs in the handler anyway, because the handler is the same code on a board.

A real product's handler usually does more: it saves the reason and the thread's name somewhere
that survives a reset, such as a retained RAM area or a setting, and resets the device, so that the
next boot can report what happened. The [`debugging`](../examples/debugging) example's handler is
[`src/fatal.c`](../examples/debugging/src/fatal.c).

---

## Assertions
An **assertion** states something the code assumes, and checks it at run time:

```c
__ASSERT((percent >= 0) && (percent <= 100), "duty cycle %d %% is out of range", percent);
```

`__ASSERT_NO_MSG(condition)` is the same without a message. A failed assertion prints the condition,
where it is, and the message, and then panics:

```text
ASSERTION FAIL [(percent >= 0) && (percent <= 100)] @ WEST_TOPDIR/lectures/L06/exercises/exercise3/src/main.c:40
	duty cycle 150 % is out of range
@ WEST_TOPDIR/deps/zephyr/lib/os/assert.c:43
[00:00:00.000,000] <err> os: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
```

**Assertions are compiled out unless `CONFIG_ASSERT=y`**, in Zephyr's code and in yours. Without it
the condition is not even evaluated, and the program above carries on with a duty cycle of 150 %.
With it, CMake warns that `__ASSERT() statements are globally ENABLED`, because every check costs
flash and time. That makes assertions a development tool: tests and debug builds enable them, and
most products ship without.

Two consequences follow:
* **An assertion must not have side effects.** `__ASSERT(k_sem_take(&sem, K_NO_WAIT) == 0, "...")`
  takes the semaphore only in builds with assertions.
* **An assertion is not error handling.** Input from a user, a sensor or the network can be wrong in
  a correct program, and has to be checked with an `if` in every build.

For conditions the compiler can decide, `BUILD_ASSERT(condition, "message")` checks at compile time
and costs nothing at run time.

---

## Sanitizers
The **sanitizers** are compiler instrumentation. The compiler adds a check to every memory access or
arithmetic operation it can, and a runtime library reports every check that fails, with the
source line, the variable, and a stack trace. They find bugs that on a board corrupt memory silently
and crash, if at all, somewhere else, much later.

### AddressSanitizer

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/debugging \
    -- -DEXTRA_CONF_FILE=asan.conf
```

`CONFIG_ASAN=y` builds with `-fsanitize=address`. AddressSanitizer surrounds every stack array, heap
block and global with poisoned *redzones*, and checks every read and write against them. It finds
out-of-bounds reads and writes, use after free, and double frees. Exercise Set 4's battery monitor
reads one element past an array on the stack:

```text
==1354852==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x78a35ddf4070 ...
READ of size 4 at 0x78a35ddf4070 thread T2
    #0 0x402a6d in average .../exercise4/src/main.c:18

Address 0x78a35ddf4070 is located in stack of thread T2 at offset 48 in frame
    #0 0x402aa0 in main .../exercise4/src/main.c:33

  This frame has 1 object(s):
    [32, 48) 'window' (line 34) <== Memory access at offset 48 overflows this variable
```

Read it from the top: what kind of access (`READ of size 4`), where in the code (line 18, in
`average()`), and whose memory it overflowed (`window`, declared on line 34). `T2` is a host thread:
every Zephyr thread on `native_sim` is one. The report ends with `SUMMARY: AddressSanitizer:
stack-buffer-overflow ... in average`, and the process exits with **status 1** at the first error.
(`CONFIG_ASAN_RECOVER`, on by default, compiles the checks so that they *could* continue, but the
runtime only does so when `ASAN_OPTIONS` contains `halt_on_error=0`.)

**AddressSanitizer exists only on `native_sim`**: `CONFIG_ASAN` depends on `ARCH_POSIX`, because the
checks call into the host's `libasan`. The Kconfig help also notes that leak detection at exit is off
for 64-bit boards built with GCC, so `native_sim/native/64` reports no leaks when the program ends.

### UndefinedBehaviorSanitizer

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/debugging \
    -- -DEXTRA_CONF_FILE=ubsan.conf
```

`CONFIG_UBSAN=y` builds with `-fsanitize=undefined`, which checks for undefined behaviour in
arithmetic and pointers: signed integer overflow, shifts by too much, division by zero, misaligned
pointers, and out-of-range enum and bool values. Exercise Set 4's power calculation overflows an
`int32_t`:

```text
.../exercise4/src/main.c:26:32: runtime error: signed integer overflow: 2250000 * 1000 cannot be represented in type 'int'
5000 mV, average 3725 mV, -2044967296 uW
```

Two things differ from AddressSanitizer:
* **The program continues**, with whatever the undefined operation produced, here a wrapped
  negative number. One run can report several problems.
* **The report goes to stderr**, not to the console's stdout, so a tool that only reads stdout never
  sees it. Twister's console harness is one such tool.

UndefinedBehaviorSanitizer is not tied to `native_sim` as tightly. Its reports need a runtime
library, the host's or picolibc's; on other targets `CONFIG_UBSAN_TRAP` turns undefined behaviour
into a CPU exception, without saying what or where.

**Both can be enabled at once** (`-DCONFIG_ASAN=y -DCONFIG_UBSAN=y`), and a program that is correct
prints exactly the same with them as without. That is how Exercise Set 4 is tested.

### What a program without them does
The same battery monitor, built without sanitizers, printed this on the machine these notes were
written on:

```text
3300 mV, average 249333220 mV, 1485000000 uW
5000 mV, average 249333645 mV, -2044967296 uW
```

The out-of-bounds read picked up whatever was next to the array, and the overflow wrapped. Neither
crashed. Another build of the same code averaged `261933924 mV` instead, and on another compiler, or
with another optimization level, the average could as easily have looked plausible. That is the case for the sanitizers: undefined behaviour does not announce
itself.

---

## GDB
A `native_sim` program is an ordinary Linux executable with debug information, so GDB debugs it
directly, with no probe and no GDB server:

```bash
sudo apt install gdb
gdb --args ./build/zephyr/zephyr.exe -no-rt
```

> **Not verified here.** GDB was not installed on the machine these notes were written on, so the
> commands in this section were not run while writing them. They are GDB's standard commands, and
> the Kconfig options they rely on were checked in Zephyr's source.

The commands that matter most:

| Command | Effect |
|---------|--------|
| `break average`, `break main.c:18` | Stop at a function, or at a line. |
| `break k_sys_fatal_error_handler` | Stop at every fatal error, before the handler decides. |
| `run`, `continue` | Start the program, or carry on after a stop. |
| `backtrace` | The calls that led to where the program stopped. |
| `print window[i]`, `info locals` | Values of variables. |
| `info threads`, `thread 2` | The host threads, and switching between them. |

Two Zephyr options make it easier:
* **`CONFIG_DEBUG_OPTIMIZATIONS=y`** compiles with `-Og`, which the
  [`debugging`](../examples/debugging) example sets, so that variables are not optimized away and
  lines run in the order they are written.
* **`CONFIG_ARCH_POSIX_TRAP_ON_FATAL=y`** raises `SIGTRAP` in the halt path, before the process exits,
  so a debugger stops at a fatal error without a breakpoint.

`native_sim` creates a host thread for every Zephyr thread when the Zephyr thread is created, next
to the simulator's own. A thread that is stalled in a busy loop (L03) is easy to find this way:
interrupt the program with `Ctrl+C` in GDB, and look at `info threads` and `backtrace`.

---

## Coverage
**Coverage** measures which lines and branches of the code a run executed. On `native_sim` it is
GCC's own gcov:

```bash
west build -p -b native_sim/native/64 lectures/L06/exercises/exercise4 \
    -- -DCONFIG_COVERAGE=y
./build/zephyr/zephyr.exe -no-rt -stop_at=1
```

With `CONFIG_COVERAGE=y` the program counts every line it runs, and **writes the counts when it
exits**: `-stop_at` is an exit, and so is a fatal error's. They go into a `.gcda` file next to each
object file, such as `build/CMakeFiles/app.dir/src/main.c.gcda`, which `gcov` reads:

```bash
cd build
gcov -o CMakeFiles/app.dir/src/main.c.gcda \
    ../lectures/L06/exercises/exercise4/src/main.c
```

```text
Lines executed:100.00% of 14
Creating 'main.c.gcov'
```

(The number of lines depends on how your `exercise4` is written.)

`main.c.gcov` is the source with a count beside every line:

```text
       24:   18:	for (size_t i = 0; i < n; i++) {
       18:   19:		sum += window[i];
```

The loop body ran 18 times over the six calls, `1 + 2 + 3 + 4 + 4 + 4`, and its condition was tested
24 times, once more per call, when it failed.

Twister collects coverage over a whole test run, and turns it into an HTML report:

```bash
sudo apt install lcov
west twister -T lectures/L06/examples/debugging -p native_sim/native/64 \
    --coverage --coverage-tool lcov
```

The report is `twister-out/coverage/index.html`. Twister's default tool is `gcovr` instead, which is
installed with `pip install gcovr`.

On a board, coverage is possible too (`CONFIG_COVERAGE_GCOV`), but the counts have to be dumped over a
serial line and converted on the host, and the instrumentation needs RAM that small devices do not
have. On `native_sim` it is one option.

---

## Summary
* A fatal error has a reason: an oops means a thread cannot go on, a panic that the system cannot.
  Assertions panic.
* The kernel logs every fatal error, and calls `k_sys_fatal_error_handler()`. The default one halts;
  on `native_sim` that is exit status 1.
* A handler of your own decides what an error costs. Returning aborts only the faulting thread; a
  panic must halt, with `k_fatal_halt()`.
* `__ASSERT()` is compiled out unless `CONFIG_ASSERT=y`. Assertions must have no side effects, and
  are not error handling.
* AddressSanitizer finds out-of-bounds access and use after free, stops at the first error, and
  exists only on `native_sim`.
* UndefinedBehaviorSanitizer finds undefined arithmetic, reports on stderr, and continues.
* A `native_sim` program is debugged with GDB like any Linux program, one host thread per Zephyr
  thread.
* `CONFIG_COVERAGE=y` writes gcov data when the program exits; Twister's `--coverage` turns a test
  run's into a report.

---
