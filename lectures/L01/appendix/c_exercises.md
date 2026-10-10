# Appendix C

## Exercises
These exercises reinforce the concepts from [Appendix B](./b_zephyr_and_the_build.md).

> **How to check your work.** Write each exercise set as its own application under
> [`lectures/L01/exercises`](../exercises/README.md), with the file names given there, and run
> `make test` from the repository root. A set's tests switch on as soon as its files exist, and a
> set you have not started is reported as `SKIP`. Each application grows through its set, and the
> tests check where it ends up. Programs run on `native_sim` in simulated time, so their output,
> timestamps included, is compared with the expected output line by line.

Remember `source env.sh` in every new terminal. Build and run an exercise from the repository root:

```bash
west build -p -b native_sim/native/64 lectures/L01/exercises/exercise1
west build -t run
```

---

# Exercise Set 1 – A first application
Create a directory named `exercise1` to work in for this exercise set.

## Exercise 1.1 – The three files
In this exercise you will create the smallest Zephyr application that builds.

### Tasks
Create these files in `exercise1`:
* `CMakeLists.txt`: require CMake 3.20, find the Zephyr package, name the project `exercise1`, and
  add `src/main.c` to the `app` target.
* `prj.conf`: an empty file, or one holding only a comment.
* `src/main.c`: a `main()` that prints `Hello, Zephyr!` with `printk()` and returns `0`.

Build and run the application.

### Reflection
* What happens if you delete `prj.conf` and build again with `-p`?
* The program prints its greeting and then keeps running. Why does it not exit when `main()`
  returns? How do you stop it?
* Where in `build/` is the program you ran?

---

## Exercise 1.2 – The uptime
### Tasks
Extend `main()` so that after the greeting it prints the uptime three times, with `k_msleep(1000)`
after each print:

```text
Hello, Zephyr!
Uptime: 0 ms
Uptime: 1010 ms
Uptime: 2020 ms
```

Use `k_uptime_get()`, which returns an `int64_t`, printed with `%lld`.

Run the program with `./build/zephyr/zephyr.exe -stop_at=5`, and then again with
`-stop_at=5 -no-rt`.

### Reflection
* Why is the second uptime 1010 ms and not 1000 ms? Find the option in `build/zephyr/.config` that
  decides it.
* What is the difference between the two runs? Is the output different?
* Would the uptimes be the same if the host were busy compiling something else at the same time?
  Why?

---

# Exercise Set 2 – Logging
Create an application named `exercise2` to work in for this exercise set, with the same three
files as `exercise1`.

## Exercise 2.1 – A temperature monitor
In this exercise you will replace `printk()` with the logging subsystem.

### Tasks
* Enable logging in `prj.conf`.
* Register a log module named `monitor` **without** giving a level:
  `LOG_MODULE_REGISTER(monitor);`.
* In `main.c`, define the readings as an array:

  ```c
  static const int readings[] = {21, 24, 28, 33, 36, 29};
  ```

* In `main()`, go through the readings, and for each one log:
  * `Temperature <t> C is critical` as an error, if it is 35 or more.
  * `Temperature <t> C is high` as a warning, if it is 30 or more.
  * `Temperature <t> C` as information, otherwise.
* Sleep 500 ms after each reading.

The output must be:

```text
[00:00:00.000,000] <inf> monitor: Temperature 21 C
[00:00:00.510,000] <inf> monitor: Temperature 24 C
[00:00:01.020,000] <inf> monitor: Temperature 28 C
[00:00:01.530,000] <wrn> monitor: Temperature 33 C is high
[00:00:02.040,000] <err> monitor: Temperature 36 C is critical
[00:00:02.550,000] <inf> monitor: Temperature 29 C
```

**Hint:** `ARRAY_SIZE(readings)`, from `<zephyr/sys/util.h>`, which `<zephyr/kernel.h>` includes,
gives the number of elements.

---

## Exercise 2.2 – Only what matters
### Tasks
Without changing any file, build the application so that it logs only warnings and errors, by
setting the default log level on the command line:

```bash
west build -p -b native_sim/native/64 lectures/L01/exercises/exercise2 -- -DCONFIG_LOG_DEFAULT_LEVEL=2
```

The output must now be only:

```text
[00:00:01.530,000] <wrn> monitor: Temperature 33 C is high
[00:00:02.040,000] <err> monitor: Temperature 36 C is critical
```

### Reflection
* Why did this change the monitor's output, when the command line said nothing about `monitor`?
* Would it still work if the module were registered as `LOG_MODULE_REGISTER(monitor, LOG_LEVEL_INF)`?
* Are the information messages still in the program, only not printed? Explain how you know.
* After this build, run `west build -t run` a few times, and then build again **without** `-p` and
  without the `-D` option. Which log level does that build use? Look for the answer in
  `build/zephyr/.config`, and explain it.

---

# Exercise Set 3 – Options of your own
Create an application named `exercise3` to work in for this exercise set.

## Exercise 3.1 – The threshold
In this exercise you will give the application Kconfig options of its own.

### Tasks
* Create a `Kconfig` file in `exercise3` with a menu named `Application`, holding:
  * `APP_HIGH_C`, an `int` with the prompt `Temperature at which a reading is high, in degrees
    Celsius`, a range from -40 to 125, and a default of 30.
* End the file so that Zephyr's own options are still available.
* In `prj.conf`, set `APP_HIGH_C` to 25.
* In `src/main.c`, go through these readings, printing each with `printk()`, and mark those at or
  above `CONFIG_APP_HIGH_C` with ` (high)`:

  ```c
  static const int readings[] = {-18, 21, 24, 28, 33};
  ```

The output must be:

```text
-18 C
21 C
24 C
28 C (high)
33 C (high)
```

### Reflection
* Find `APP_HIGH_C` in `west build -t menuconfig`. What does `?` show about it?
* Set `CONFIG_APP_HIGH_C=200` in `prj.conf` and build. What happens, and why is that better than
  the alternative?
* Remove the last line of your `Kconfig` file and build with `-p`. What happens?

---

## Exercise 3.2 – Fahrenheit, when asked for
### Tasks
* Add a `bool` option `APP_FAHRENHEIT` to the `Application` menu, with the prompt `Print
  temperatures in degrees Fahrenheit`.
* Create `src/convert.h` and `src/convert.c` with a function:

  ```c
  int celsius_to_fahrenheit(int celsius);
  ```

  which returns the temperature in degrees Fahrenheit, **rounded to the nearest degree**, using only
  integer arithmetic. For example, 37 °C is 98.6 °F, which rounds to 99, and -18 °C is -0.4 °F,
  which rounds to 0.
* In `CMakeLists.txt`, compile `src/convert.c` **only** when `CONFIG_APP_FAHRENHEIT` is enabled.
* In `main()`, when the option is enabled, print each reading in Fahrenheit, as `<t> F`, still
  marked ` (high)` by its value in Celsius. Use `IS_ENABLED()`, not `#ifdef`.
* Create a file `fahrenheit.conf` in `exercise3` that enables the option.

Built with `-DEXTRA_CONF_FILE=fahrenheit.conf`, the output must be:

```text
0 F
70 F
75 F
82 F (high)
91 F (high)
```

Built without it, the output must be unchanged from Exercise 3.1.

**Hint:** Work in tenths of a degree: 37 °C is 986 tenths of a degree Fahrenheit. C's integer
division truncates towards zero, so `(tenths + 5) / 10` rounds a positive number of tenths to the
nearest degree, but not a negative one.

### Reflection
* Without `fahrenheit.conf`, `convert.c` is not compiled, yet `main.c` still calls
  `celsius_to_fahrenheit()`. Why does the program link?
* Would it still link with `#ifdef CONFIG_APP_FAHRENHEIT` around the call instead? Which of the two
  would you rather maintain, and why?
* Change `IS_ENABLED(CONFIG_APP_FAHRENHEIT)` to `IS_ENABLED(CONFIG_APP_FARENHEIT)` and build with
  `fahrenheit.conf`. What happens? What would have caught it?

---

# Exercise Set 4 – Reading the workspace and the build
No code in this set: answer the questions by looking. Use the build of any exercise above.

## Exercise 4.1 – The workspace
### Tasks
Answer, using west's commands rather than by opening files:
* Which revision of Zephyr is the workspace using, and where is it checked out?
* Which configuration value makes `west build` work without `-b`?

---

## Exercise 4.2 – The build
### Tasks
Answer, by looking in the build directory:
* Which log mode (`CONFIG_LOG_MODE_...`) did the `exercise2` build use? Why that one?
* What is the value of `CONFIG_BOARD`, and what is the value of `CONFIG_BOARD_TARGET`? Why are they
  different?
* How many `#define` lines does `autoconf.h` hold? Does it contain the options that are `n`?
* Find the definition of `k_msleep()` with
  `grep -n -A3 "k_msleep(int32_t ms)" deps/zephyr/include/zephyr/kernel.h`. What does it call, and
  what does `static inline` mean for whether `k_msleep` exists as a function in your program?

---
