# Appendix C

## Exercises
These exercises reinforce the concepts from [Appendix A](./a_subsystems.md) and
[Appendix B](./b_debugging.md).

> **How to check your work.** Write each exercise set as its own application under
> [`lectures/L06/exercises`](../exercises/README.md), with the file names given there, and run
> `make test` from the repository root. A set's tests switch on as soon as its files exist, and a
> set you have not started is reported as `SKIP`. Each application grows through its set, and the
> tests check where it ends up. Shell commands are fed to your program on its standard input, and
> its output is checked for the lines each exercise shows, in order; the prompt, the echoed commands
> and anything else in between do not matter.

Remember `source env.sh` in every new terminal. Build and run an exercise from the repository root:

```bash
west build -p -b native_sim/native/64 lectures/L06/exercises/exercise1
west build -t run
```

---

# Exercise Set 1 – Shell commands of your own
Create an application named `exercise1` to work in for this exercise set.

## Exercise 1.1 – The setpoint
In this exercise you will give a thermostat a shell command that reads and changes its setpoint.

### Tasks
* In `prj.conf`, enable the shell, and put the shell's UART on the terminal the program is started
  from.
* In `src/main.c`, keep the setpoint in a `static int setpoint_c`, initially 21.
* Create a root command `setpoint` with the help text `The thermostat's setpoint.`, no handler of its
  own, and two subcommands:
  * `get`, which takes no arguments, and prints `Setpoint: <t> C`.
  * `set <celsius>`, which takes exactly one argument, sets the setpoint, and prints
    `Setpoint set to <t> C`.
* Let the shell check the argument counts: declare them with `SHELL_CMD_ARG()`.
* `main()` prints `Thermostat ready` and returns.

Pipe a session into the program:

```bash
printf 'setpoint get\nsetpoint set 25\nsetpoint get\n' \
    | ./build/zephyr/zephyr.exe -stop_at=1
```

The output must contain, in this order:

```text
Thermostat ready
Setpoint: 21 C
Setpoint set to 25 C
Setpoint: 25 C
```

### Reflection
* The program's output also contains `uart:~$ setpoint get`. Where does that line come from?
* Run `setpoint` alone, and then `retval`. What does `retval` print, and why that value?
* `main()` returns at once. Why does the shell keep answering commands?

---

## Exercise 1.2 – Wrong input
### Tasks
Make `setpoint set` reject what is not a valid setpoint:
* A word that is not a whole decimal number, such as `warm` or `25x`, prints the error
  `not a temperature: '<word>'` with `shell_error()` and returns `-EINVAL`.
* A number outside 5 to 30 prints the error `setpoint must be between 5 and 30 C, not <t>` and
  returns `-ERANGE`. The setpoint does not change.

For the session

```text
setpoint get
setpoint set 25
setpoint get
setpoint set 31
retval
setpoint set warm
retval
setpoint set 25x
setpoint set
```

the output must contain, in this order:

```text
Setpoint: 21 C
Setpoint set to 25 C
Setpoint: 25 C
setpoint must be between 5 and 30 C, not 31
-34
not a temperature: 'warm'
-22
not a temperature: '25x'
set: wrong parameter count
```

**Hint:** `strtol()` from `<stdlib.h>` sets its `end` argument to the first character it did not
convert.

### Reflection
* Why is `atoi()` not good enough here?
* Which part of the output does the shell print by itself, without your handler being called?
* `setpoint set 99999999999999999999` is a number too. What does your handler do with it, and why?

---

# Exercise Set 2 – A setpoint that survives a restart
Copy `exercise1` to a new application named `exercise2` to work in for this exercise set.

## Exercise 2.1 – Storing it
In this exercise you will store the setpoint with the settings subsystem, on the simulated flash.

### Tasks
* In `prj.conf`, enable the settings subsystem with the ZMS backend, and what ZMS needs: flash and
  the flash map.
* Keep the setpoint in an `int32_t`, stored under the name `thermostat/setpoint`.
* Register a settings handler for the subtree `thermostat` with `SETTINGS_STATIC_HANDLER_DEFINE()`.
  Its `h_set` callback takes the stored `setpoint`, and rejects a stored value whose length is not
  that of an `int32_t` with `-EINVAL`.
* In `main()`, initialize the settings subsystem, load the settings, and print
  `Loaded setpoint: <t> C` instead of `Thermostat ready`.
* `setpoint set` stores the new value with `settings_save_one()` before it changes the setpoint in
  RAM. If storing fails, it prints `cannot store the setpoint: <error>` with `shell_error()` and
  returns the error.

Run the program twice, from the same directory:

```bash
cd build
printf 'setpoint set 25\n' | ./zephyr/zephyr.exe -stop_at=1
printf 'setpoint get\n' | ./zephyr/zephyr.exe -stop_at=1
```

The first run must print `Loaded setpoint: 21 C` and `Setpoint set to 25 C`, and the second
`Loaded setpoint: 25 C` and `Setpoint: 25 C`.

### Reflection
* Where is the setpoint stored? Find the file, and find the setting's name in it with `strings`.
* Run the second command again from the repository root instead of from `build/`. What does it load,
  and why?
* Run it with `-flash_in_ram`. What does it load, and has the file changed?
* In `h_set`, why read the value into a local variable and check it, rather than straight into the
  setpoint?

---

## Exercise 2.2 – Forgetting it
### Tasks
Add a subcommand `setpoint reset`, which takes no arguments, deletes the stored setpoint with
`settings_delete()`, sets the setpoint back to 21, and prints `Setpoint reset to 21 C`.

In five runs from the same directory, where each run is given the commands shown, the output must
contain:

| Run | Commands | Output |
|-----|----------|--------|
| 1 | `setpoint set 25` | `Loaded setpoint: 21 C`, then `Setpoint set to 25 C` |
| 2 | `setpoint get` | `Loaded setpoint: 25 C`, then `Setpoint: 25 C` |
| 3 | none, run with `-flash_in_ram` | `Loaded setpoint: 21 C` |
| 4 | `setpoint reset` | `Loaded setpoint: 25 C`, then `Setpoint reset to 21 C` |
| 5 | none | `Loaded setpoint: 21 C` |

### Reflection
* Enable `CONFIG_SETTINGS_SHELL` on the command line, and run `settings list` and
  `settings read thermostat/setpoint` after setting the setpoint to 27. What do the four bytes it
  prints mean?
* How would you make every run of the tests start from a factory-new device, without deleting files
  by hand?

---

# Exercise Set 3 – Fatal errors
Create an application named `exercise3` to work in for this exercise set.

## Exercise 3.1 – An impossible reading
In this exercise you will make a thread give up with `k_oops()`, and see what that costs without a
handler of your own.

### Tasks
* In `prj.conf`, enable logging and thread names.
* In `src/main.c`, register the log module `app`, and define the readings:

  ```c
  static const int readings[] = {21, 22, -300, 23};
  ```

* Write a thread function `sampler()` that goes through the readings. A reading below -273 logs the
  error `Impossible reading: <t> C` and calls `k_oops()`; any other logs `Sample <t> C`.
* In `main()`, create the thread with `k_thread_create()`, with a 1024-byte stack from
  `K_THREAD_STACK_DEFINE()` and priority 5, name it `sampler`, and wait for it with
  `k_thread_join()`. Then log `main() carries on`, and return.

Build, run, and check the exit status:

```bash
./build/zephyr/zephyr.exe -stop_at=1; echo "exit status $?"
```

### Reflection
* Which lines does the kernel print, and what does each one tell you?
* Does `main() carries on` appear? What is the exit status, and why?
* Build once more with `CONFIG_LOG=n` on the command line. What does a fatal error print now?

---

## Exercise 3.2 – A handler of your own
### Tasks
* Create `src/fatal.c`, with a log module named `fatal`, and define `k_sys_fatal_error_handler()` in
  it. It shall:
  * Switch logging to synchronous with `LOG_PANIC()`.
  * Log the error `Fatal error <reason> in thread '<name>'`.
  * For an oops, log `Aborting the thread` and return.
  * For anything else, log `Halting the system` and call `k_fatal_halt()`.
* Add `src/fatal.c` to the application in `CMakeLists.txt`.

The output must now contain, in this order, and the exit status must be 0:

```text
<inf> app: Sample 21 C
<inf> app: Sample 22 C
<err> app: Impossible reading: -300 C
>>> ZEPHYR FATAL ERROR 3: Kernel oops
<err> fatal: Fatal error 3 in thread 'sampler'
<err> fatal: Aborting the thread
<inf> app: main() carries on
```

### Reflection
* Why does `Sample 23 C` never appear?
* Why does `k_thread_join()` return, when the thread never returned from its function?
* Remove the check for the oops, so that the handler always returns. Nothing changes in this
  program's output. Why not, and why is it still wrong?

---

## Exercise 3.3 – An assertion
### Tasks
* In `src/main.c`, add a function:

  ```c
  static void set_heater_duty(int percent)
  {
  	__ASSERT((percent >= 0) && (percent <= 100), "duty cycle %d %% is out of range", percent);
  	LOG_INF("Heater duty cycle %d %%", percent);
  }
  ```

* At the end of `main()`, after `main() carries on`, call `set_heater_duty(150)`.

Built as usual, the program must also print `<inf> app: Heater duty cycle 150 %` at the end, and exit
with status 0. Built with `-DCONFIG_ASSERT=y`, the output must contain, in this order, and the exit
status must be 1:

```text
<inf> app: main() carries on
duty cycle 150 % is out of range
>>> ZEPHYR FATAL ERROR 4: Kernel panic
<err> fatal: Fatal error 4 in thread 'main'
<err> fatal: Halting the system
```

### Reflection
* Why does the same program pass a duty cycle of 150 % in one build and halt in the other?
* With the handler from the last reflection of Exercise 3.2, the one that always returns, build with
  `-DCONFIG_ASSERT=y` and run. What happens, and what is the exit status?
* Why would `__ASSERT(settings_save_one(...) == 0, "...")` be a bug?

---

# Exercise Set 4 – Sanitizers
Create an application named `exercise4` to work in for this exercise set.

## Exercise 4.1 – A battery monitor
In this exercise you will find two bugs with the sanitizers, and fix them.

### Tasks
Create `CMakeLists.txt`, a `prj.conf` with only a comment, and a `src/main.c` holding exactly this:

```c
/*
 * SPDX-License-Identifier: MIT
 *
 * Exercise Set 4: a battery monitor, with two bugs.
 */

#include <zephyr/kernel.h>

/* How many of the most recent readings the average is taken over. */
#define WINDOW 4

/* The average of the first n readings in a window. */
static __noinline int32_t average(const int32_t *window, size_t n)
{
	int32_t sum = 0;

	for (size_t i = 0; i <= n; i++) {
		sum += window[i];
	}
	return sum / (int32_t)n;
}

/* The power drawn at a voltage and a current, in microwatts. */
static __noinline int32_t power_uw(int32_t millivolts, int32_t milliamps)
{
	return millivolts * milliamps * 1000;
}

static const int32_t readings_mv[] = {3300, 3310, 3290, 3300, 5000, 5020};
static const int32_t load_ma = 450;

int main(void)
{
	int32_t window[WINDOW] = {0};

	for (size_t i = 0; i < ARRAY_SIZE(readings_mv); i++) {
		window[i % WINDOW] = readings_mv[i];

		printk("%d mV, average %d mV, %d uW\n", readings_mv[i],
		       average(window, MIN(i + 1, WINDOW)), power_uw(readings_mv[i], load_ma));
	}
	return 0;
}
```

Build and run it as it is.

### Reflection
* Which lines of the output are wrong? Would you have noticed if you did not know what to expect?
* The first three lines are right, although the bug in `average()` is there from the first call.
  Why do they come out right?

---

## Exercise 4.2 – AddressSanitizer
### Tasks
Build with `-DCONFIG_ASAN=y`, run, and fix the bug that AddressSanitizer reports, changing as little
as possible.

### Reflection
* Which line, which variable, and which kind of access does the report name?
* What is the exit status, and why do the lines after the report never appear?

---

## Exercise 4.3 – UndefinedBehaviorSanitizer
### Tasks
Build with `-DCONFIG_UBSAN=y`, run, and fix the bug that UndefinedBehaviorSanitizer reports, so that
`power_uw()` returns an `int64_t` and computes the power without overflow for any voltage and current
that fit in an `int32_t` each.

Built with `-DCONFIG_ASAN=y -DCONFIG_UBSAN=y`, and built without either, the program must print
exactly:

```text
3300 mV, average 3300 mV, 1485000000 uW
3310 mV, average 3305 mV, 1489500000 uW
3290 mV, average 3300 mV, 1480500000 uW
3300 mV, average 3300 mV, 1485000000 uW
5000 mV, average 3725 mV, 2250000000 uW
5020 mV, average 4152 mV, 2259000000 uW
```

### Reflection
* The program did not stop at the report. How is that different from AddressSanitizer?
* Would `return (int64_t)(millivolts * milliamps * 1000);` fix the bug? Try it with UBSan.
* Build the fixed program with `-DCONFIG_COVERAGE=y`, run it, and run `gcov` on `main.c` as in
  Appendix B. How many times did the body of the loop in `average()` run, and why that number?

---

# Exercise Set 5 – Sockets
Create an application named `exercise5` to work in for this exercise set.

## Exercise 5.1 – A UDP server
In this exercise you will write a UDP server that answers every datagram in capitals, and a client
for it in the same program, talking through the host's network with offloaded sockets.

### Tasks
* In `prj.conf`, enable networking with IPv4 only, sockets, and `native_sim`'s offloaded sockets, as
  in the [`sockets`](../examples/sockets) example. No logging is needed.
* Define `PORT` as 4300.
* Write a thread `server`, with `K_THREAD_DEFINE()`, a 2048-byte stack and priority 7, which:
  * Creates a UDP socket, and binds it to `PORT` on every address (`NET_INADDR_ANY`).
  * In an endless loop, receives a datagram of up to 64 bytes with `zsock_recvfrom()`, turns it into
    capitals with `toupper()`, and sends it back with `zsock_sendto()` to the address it came from.
* Use the prefixed names throughout: `struct net_sockaddr_in`, `NET_AF_INET`, `net_htons()`, and so
  on.

---

## Exercise 5.2 – The client
### Tasks
In `main()`:
* Create a UDP socket, and set its receive timeout to one second with `ZSOCK_SO_RCVTIMEO`.
* Sleep 100 ms, so that the server, which has a lower priority, has bound its port.
* For each of the words `hello`, `zephyr` and `native_sim`, send the word to `127.0.0.1` at `PORT`,
  receive the reply, and print `Sent '<word>', received '<reply>'`. If no reply arrives, print
  `No reply to '<word>': <errno>` instead.
* Close the socket and print `Done`.

The output must contain, in this order:

```text
Sent 'hello', received 'HELLO'
Sent 'zephyr', received 'ZEPHYR'
Sent 'native_sim', received 'NATIVE_SIM'
Done
```

### Reflection
* Run the program without `-no-rt` and with `-stop_at=10`, and send it a datagram from another
  terminal with `printf 'from the host' | nc -u -w1 127.0.0.1 4300`. What comes back, and what does
  that say about where the port is?
* Why does the client set a receive timeout, when the example's TCP client does not?
* The output also begins with `WARNING: Using a test - not safe - entropy source`. Where does that
  come from?

---
