# Paper A - Solutions

Marks are shown per part. Method carries them: a correct diagnosis with a clumsy fix is worth more
than a correct fix with no diagnosis, and several later parts consume earlier answers, so an error
should be followed through rather than penalised twice.

Code is marked on semantics. Missing includes, missing semicolons and misplaced braces cost nothing.

---

## Question 1 - The workspace and the build (12 marks)

### (a) 3 marks

**What the build is looking for.** Zephyr chooses a toolchain before it compiles anything. If
`ZEPHYR_TOOLCHAIN_VARIANT` is not set, it assumes the Zephyr SDK — the cross-compilers for real
microcontrollers — and asks CMake to find the SDK's package configuration file. Nothing has
installed one, so configuration fails before a single source file is read.
*(1 mark)*

**The fix** is to source the course's environment file in that terminal:

```bash
source env.sh
```

It activates the Python environment that holds west and Twister, and sets two variables:
`ZEPHYR_BASE`, which says where Zephyr is (`deps/zephyr`), and `ZEPHYR_TOOLCHAIN_VARIANT=host`,
which tells Zephyr to build with the host's own GCC.
*(1 mark)*

**Why no SDK is needed.** A `native_sim` image is an ordinary Linux executable: the kernel, the
drivers and the application are compiled for the host CPU by the host compiler. The SDK exists to
cross-compile for a target, and this course never leaves the host.
*(1 mark)*

`make setup` having finished is a red herring, and saying so earns the first mark: setup creates
`.venv`, writes `.west/config` and fetches Zephyr, all of which persist, whereas `source env.sh` is
per terminal and is what the message is complaining about.

### (b) 3 marks

**The three files**, and what each contributes:

* **`CMakeLists.txt`** — how to build it: it finds the Zephyr package and adds the application's own
  sources to the `app` target.
* **`prj.conf`** — how to configure Zephyr for it: a list of Kconfig settings applied on top of the
  board's defaults. It must exist even if it is empty, which is exactly what this error says.
* **`src/`** — the code, with a `main()`.

*(1.5 marks, half for naming all three, half for what they do.)*

**The line that does most of the work** is:

```cmake
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
```

It is where Zephyr's build system takes over: it reads the board, processes the devicetree, runs
Kconfig, sets Zephyr up as a set of CMake libraries, and creates the `app` target.
*(1 mark)*

**Why `project()` comes after it.** Zephyr selects the compiler as part of that call. CMake's
`project()` is what makes CMake look for a compiler, so it must not run until the choice has been
made.
*(0.5 marks)*

### (c) 4 marks

**What `west update` does.** It reads the manifest and makes the workspace match it: it clones the
`zephyr` project from `https://github.com/zephyrproject-rtos/zephyr` — the remote's `url-base`
followed by the project's name — and checks out the revision `v4.4.2`. With `clone-depth: 1` it
fetches only that commit rather than Zephyr's history. `path: deps/zephyr` puts it in
`deps/zephyr`, inside the repository, and `self: path: .` says the manifest repository is the
workspace directory itself.
*(2 marks)*

**Without `west-commands`.** West would know only its own built-in commands. `west build` and
`west twister` are extension commands that Zephyr provides, and that line is what tells west where
Zephyr declares them.
*(1 mark)*

**Moving to a newer release** is two steps: change `revision:` in `west.yml`, and run `west update`.
West never moves a project on its own; pulling the course repository changes the manifest, and
`west update` is what makes the checkout match it.
*(0.5 marks)*

**Why Zephyr's own manifest is not imported.** It lists some eighty modules — vendor HALs,
cryptography libraries, file systems — none of which `native_sim` needs, and together they are
several gigabytes. A product would import the few it needs by name.
*(0.5 marks)*

### (d) 2 marks

**Why the level is still 2.** A `-DCONFIG_...` argument is stored in the build directory's CMake
cache, and every later build in that directory keeps using it until the directory is rebuilt
pristine or the option is given a new value. The second build inherited it.
*(1 mark)*

**What proves the configuration.** Read it back from the build:

```bash
grep CONFIG_LOG_DEFAULT_LEVEL build/zephyr/.config
```

`build/zephyr/.config` holds every option and the value the build actually used. Credit
`west build -t menuconfig` as an alternative, as long as the candidate says it shows the same file's
contents. A pristine build (`west build -p`) is the fix, not the proof.
*(1 mark)*

---

## Question 2 - Kconfig and logging (12 marks)

### (a) 4 marks

**What Kconfig did.** An assignment in `prj.conf` is a *request*, not a command. `SHELL_LOG_BACKEND`
depends on `SHELL`, which is `n`, so the symbol cannot be `y` and Kconfig left it at `n`. The
warning says exactly that: assigned `y`, got `n`, because of the unsatisfied dependency `SHELL (=n)`.
*(1.5 marks)*

**Why the build stops.** Zephyr turns that warning into an error. The alternative is an image built
without the feature that `prj.conf` plainly asks for, and nobody finds out until the behaviour is
missing at run time. Failing the build makes a request that did not take impossible to ignore.
*(1 mark)*

**Two ways to succeed**, either of which is the right answer depending on intent:

* Enable the dependency as well: `CONFIG_SHELL=y` alongside it.
* Remove the assignment, having decided the feature is not wanted.

*(1 mark)*

**Where to read the result.** `build/zephyr/.config`. The same values reach C through
`build/zephyr/include/generated/zephyr/autoconf.h`.
*(0.5 marks)*

### (b) 3 marks

**What the compiler did.** Nothing at all. `CONFIG_APP_FARENHEIT` is not defined, and an undefined
macro inside `IS_ENABLED()` is indistinguishable from an option that is off, so the expression is
the constant `0` and the compiler removes the branch. There is no diagnostic, because nothing is
wrong as far as C is concerned.
*(1 mark)*

**The consequence.** The feature can never be switched on. `CONFIG_APP_FAHRENHEIT=y` in `prj.conf`
takes effect in the configuration, `.config` shows it enabled, and the program still prints Celsius.
The build and the configuration both look right; only the behaviour is wrong.
*(1 mark)*

**Why the same misspelling in `prj.conf` would not survive.** Kconfig knows which symbols exist. An
assignment to a symbol that does not exist is a warning, and Zephyr aborts the build on it, exactly
as in part (a).
*(0.5 marks)*

**What would have caught it.** A test that checks the program's output in both configurations — the
course's own exercise suites do this, building each set twice. Credit also Zephyr's compliance check
for undefined Kconfig references, and credit a candidate who says that a code review looking for
`CONFIG_` names not present in `.config` would find it.
*(0.5 marks)*

### (c) 3 marks

**What is printed.** The error and the warning only. Level 2 is `WRN`, and the levels run
`NONE` 0, `ERR` 1, `WRN` 2, `INF` 3, `DBG` 4. A module registered without a level takes
`CONFIG_LOG_DEFAULT_LEVEL`, so this module's level is 2.
*(1 mark)*

**Whether the others are in the binary.** No. A module's level is a **compile-time** filter: the
`LOG_INF()` and `LOG_DBG()` calls expand to nothing, and their format strings are not in the image
either. That is why debug logging can be left in the source for free, and why turning it on needs a
rebuild rather than a setting.
*(1 mark)*

**How to demonstrate it.** Look for the format strings in the executable:

```bash
strings build/zephyr/zephyr.exe | grep "Temperature"
```

The two surviving messages are there; the filtered ones are not. Credit any equivalent: comparing
the two builds' sizes, or inspecting the map file.
*(1 mark)*

### (d) 2 marks

* **Immediate** (`native_sim`'s default): the message is formatted and written by the thread that
  logged it, in order with `printk()` and with everything else. The cost is paid at the call, which
  on a UART would be milliseconds inside whatever was running.
* **Deferred** (the default on a board): the call only copies the arguments into a buffer, and a
  low-priority log thread formats and prints later. Logging from an interrupt handler is then cheap,
  but a message and a `printk()` after it can appear in the opposite order, and messages still in
  the buffer are lost if the system dies.

*(1 mark each. Half marks for naming the modes without a consequence.)*

---

## Question 3 - Devicetree, bindings and overlays (13 marks)

### (a) 5 marks

**What `32` is.** A node's **ordinal**: the number the build gives each devicetree node, in
dependency order. `__device_dts_ord_32` is the name of the device object for node 32. Ordinals
belong to one build: the same node can be 22 in one application and 32 in another, so the number
means nothing until it is looked up in *that* build's
`build/zephyr/include/generated/zephyr/devicetree_generated.h`, which lists the ordinal beside the
node's path.
*(2 marks)*

**The first message is a compile error.** The device object was never *declared*, which means the
build generated nothing for that node: the node is disabled, or absent. `DEVICE_DT_GET()` expanded
to the address of a symbol that does not exist. The "did you mean" is noise — 38 is simply the
nearest number that does exist.
*(1 mark)*

**The second is a link error.** The name was declared, so the node exists and is enabled, but no
translation unit ever *defined* that device: no driver was built for it. The node is there, the
driver is not.
*(1 mark)*

**One change for each.**

* First: disable the node, for example an overlay containing

  ```dts
  &gpio0 {
  	status = "disabled";
  };
  ```

* Second: leave the node enabled and remove the driver, for example by building with
  `-DCONFIG_GPIO=n`.

*(1 mark for both; half for one.)*

The pair is worth learning as a pair: *undeclared* means look at the devicetree, *undefined
reference* means look at Kconfig.

### (b) 4 marks

**The overlay that omits `threshold-c`** fails during configuration, when the tree is checked
against its bindings, before any C is compiled:

```text
devicetree error: 'threshold-c' is marked as required in 'properties:' in
'.../qacademy,alarm.yaml', but does not appear in <Node /alarm0 in .../app.overlay:12>
```

*(1.5 marks)*

**The overlay that adds `hysteresis-c`** configures without complaint: the property is in the tree
but the binding does not declare it, so no macro is generated for it. The failure comes later, when
the C is compiled and `DT_PROP()` expands to a name that does not exist:

```text
error: 'DT_N_S_alarm0_P_hysteresis_c' undeclared here (not in a function)
```

*(1.5 marks)*

**Why it is a different kind of mistake from a misspelled `CONFIG_` macro.** Both are typos that the
tool cannot know were meant to be something else, but the devicetree one **fails loudly at compile
time**, because a missing property macro is an undeclared identifier. A misspelled `CONFIG_` macro
inside `IS_ENABLED()` is silently `0` and the program builds and runs with the feature off. The
devicetree's generated macros are named entities; Kconfig's are conditional compilation.
*(1 mark)*

### (c) 2 marks

**What `GPIO_ACTIVE_LOW` changes.** Nothing in the C code, which is the point: the flag lives in the
devicetree, and the GPIO API's `_dt` functions apply it. The code says *active* and *inactive* —
`gpio_pin_get_dt()` returns 1 when the button is pressed, `GPIO_INT_EDGE_TO_ACTIVE` is the interrupt
for a press — and the driver turns that into the electrical level the board actually uses.
*(1 mark)*

**What `gpio_emul_input_set()` needs.** The **physical** level, not the logical one. The emulator
sits where the pin is, below the flags, so pressing an active-low button means driving the pin to 0
and releasing it means 1.
*(1 mark)*

### (d) 2 marks

**A portable sample uses an alias**, such as `led0` or `sw0`. An alias is a name in the tree's
`aliases` node that points at whatever node plays that role on this board, so the same source builds
for any board that defines it. A node label (`&gpio0`) and a node name (`gpio_emul`) are properties
of one particular tree, so code that uses them is tied to that board.
*(1 mark)*

**`DT_NODELABEL()`** turns a label into a node identifier, for the cases where you do mean one
specific node: an overlay's own node, or a peripheral an application is written against.
*(1 mark)*

---

## Question 4 - Threads, scheduling and time (13 marks)

### (a) 6 marks

**Why `coop_b` does not run when its semaphore is given.** `coop_a` has a cooperative priority
(negative). A cooperative thread cannot be preempted: once it has the CPU it keeps it until it gives
it up by blocking, sleeping or yielding. Giving a semaphore makes `coop_b` *ready*, and ready is not
running. `coop_b` has the higher priority of the two, so it is first in the queue, but nothing takes
the CPU away from `coop_a` to hand it over.
*(2 marks)*

**Why `high` does.** `low` is preemptible (priority 5, non-negative). The moment `k_sem_give()`
makes a higher-priority thread ready, the scheduler switches — inside the call, before it returns.
The trace shows that directly: `high`'s line is printed between `low`'s "giving" line and its "back
again" line.
*(1.5 marks)*

**What happened between the third and fourth lines of Act 1.** `coop_a` gave up the CPU
deliberately: it blocked, slept or called `k_yield()`. The fourth line says "because coop_a
yielded", so the answer expected is `k_yield()`, and credit any of the three if the candidate says
the switch cannot happen without one of them.
*(1 mark)*

**Which priorities are which, and what it means for a critical section.** Negative priorities are
cooperative and non-negative ones are preemptible. A cooperative thread's critical section is
protected from every other thread by the fact that it runs, but not from interrupts, and it delays
every higher-priority thread for as long as it runs — which is why cooperative threads are for short
work and why a cooperative thread that blocks gives all of that protection away at the moment it
blocks.
*(1.5 marks)*

### (b) 4 marks

**Why the sleep ends at 510 ms.** Timeouts are kept in ticks, and `native_sim` has 100 ticks per
second, so 500 ms is 50 ticks. A thread calling `k_msleep()` cannot know how much of the current
tick has already passed, so the kernel adds one tick to be sure the sleep is never *shorter* than
asked: 51 ticks, 510 ms. A sleep lasts at least the time requested, and up to one tick longer.
*(2 marks)*

**Why the timer fires at exactly 500 ms.** `k_timer_start()` compensates: it subtracts that extra
tick from the duration before handing it to the timeout machinery, precisely so that a timer expires
when the application said it should rather than one tick late.
*(1 mark)*

**What a periodic timer does about it.** Nothing accumulates: the period is measured from the
scheduled expiry, not from when the handler ran, so a periodic timer does not drift — 100, 200, 300,
even if a handler is late.
*(1 mark)*

### (c) 3 marks

**What returning from `main()` ends.** The `main` thread, and nothing else. `main()` is just the
entry point of one thread; the kernel, every other thread and every timer keep running. With nothing
else to do, the idle thread runs. A microcontroller has nothing to return *to*, so there is no
notion of the system exiting.
*(1.5 marks)*

**Making it stop by itself.** Run it with `-stop_at=<seconds>`, which stops the program after that
much simulated time. Credit `Ctrl+C` as the interactive answer, and credit a candidate who mentions
that the program's own code could call `k_sys_fatal_error_handler`-style exits, but `-stop_at` is
the one the course uses.
*(1 mark)*

**What `-no-rt` changes.** How fast simulated time advances relative to the host's clock: without it
the simulator paces itself to real time, with it the run finishes as fast as the host can manage.
What it does not change is a single thing the program sees — the uptimes, the order of events and
the output are identical, because everything the program observes is simulated time.
*(0.5 marks)*

---

## Question 5 - Interrupt context, work queues and simulated time (12 marks)

### (a) 5 marks

A `k_timer` expiry function runs in **interrupt context**, from the system clock's interrupt. Three
defects follow from that:

1. **`k_msleep(5)`.** Sleeping in an ISR is illegal: there is no thread to suspend. With
   `CONFIG_ASSERT=y` the kernel catches it and panics; without assertions it is undefined behaviour
   in the scheduler. *(1.5 marks)*
2. **`k_mutex_lock(&bus_mutex, K_FOREVER)`.** A mutex has an owner and may block, so it may not be
   taken from an ISR at all; a timeout other than `K_NO_WAIT` is doubly illegal there. *(1.5 marks)*
3. **The work itself.** `read_sensor()` and `store()` are bus traffic and storage in interrupt
   context: everything in the system, including higher-priority threads, waits for them. `LOG_INF()`
   belongs to the same complaint — cheap under deferred logging, not free, and on `native_sim`
   immediate. *(1 mark)*

**The shape it should have.** The expiry function does one thing: it submits a work item and
returns.

```c
static void sample_work_handler(struct k_work *work)
{
	struct sample sample;

	k_mutex_lock(&bus_mutex, K_FOREVER);
	read_sensor(&sample);
	k_mutex_unlock(&bus_mutex);
	store(&sample);
}

K_WORK_DEFINE(sample_work, sample_work_handler);

static void sample_expiry(struct k_timer *timer)
{
	k_work_submit(&sample_work);
}
```

The handler runs in a work queue's thread, where sleeping, locking and logging are all allowed.
*(1 mark)*

### (b) 3 marks

**A 200 ms work item.** The work queue is one thread running items one after another, so every other
item queued on it waits 200 ms. Nothing else on that queue can overtake it, whatever its urgency.
*(1 mark)*

**An item that blocks forever.** The system work queue is dead: no further item on it ever runs.
Because it is cooperative, it also keeps the CPU from every preemptible thread until it blocks — and
a blocked queue at least yields, which is the one mercy. Anything in Zephyr that uses the system
queue behind your back stops working too.
*(1 mark)*

**When to start your own queue.** When the work must not be delayed by — or must not delay — other
users of the system queue: work that blocks, work that takes long, or work that needs a priority of
its own. One condition stated clearly earns the mark.
*(1 mark)*

### (c) 4 marks

**What `native_sim` does.** It hangs, permanently. Code runs in zero simulated time, and the
simulator advances the clock and delivers interrupts only when every Zephyr thread is waiting. A
loop that never calls into the kernel never lets that happen, so the interrupt that would set
`ready` can never arrive, and no other thread runs either. `-stop_at` cannot end it, because the
simulated time it is waiting for never arrives.
*(2 marks)*

**What it costs on real hardware.** It works, and it is still bad: the CPU spins at full power until
the interrupt arrives, delaying nothing but wasting everything. The interrupt is real there, so it
does preempt the loop.
*(1 mark)*

**Two correct rewrites.** Any two of:

* A semaphore: the ISR calls `k_sem_give(&ready_sem)` and `main()` calls
  `k_sem_take(&ready_sem, K_FOREVER)`.
* `k_poll()` on a `k_poll_signal` raised by the ISR, which generalises to waiting for several
  things.
* As a last resort, keeping the loop but letting the CPU idle inside it, with `k_cpu_idle()` or a
  short `k_busy_wait()`; the course's own note calls this the workaround rather than the fix.

*(1 mark for two.)*

---

## Question 6 - Synchronization and communication (13 marks)

### (a) 6 marks

**The phenomenon is priority inversion**, and the unbounded kind: the highest-priority thread in the
system waits for a thread that is not even using the lock.
*(1 mark)*

**579 ms, with the semaphore.** `high` asks at 21 ms and gets the lock at 600 ms. `low` had done
31 ms of its 100 ms of work when `medium` woke at 31 ms and preempted it — `medium` has the higher
priority and needs no lock — and ran to 531 ms. Only then did `low` get the CPU back to finish its
remaining 69 ms, reaching 600 ms, where it released the lock. So `high`'s wait is `low`'s critical
section *plus the whole of medium's unrelated work*: 600 − 21 = 579.
*(2 marks)*

**79 ms, with the mutex.** `high` asks at 21 ms and gets the lock at 100 ms, which is exactly the
rest of `low`'s 100 ms critical section: 100 − 21 = 79. `medium` never gets in.
*(1 mark)*

**What the kernel did to `low`.** Priority inheritance. When `high` blocked on the mutex at 21 ms,
the kernel raised the owner's priority to the waiter's, so `low` ran at priority 5 — the trace shows
it, `low (priority 5)` — and `medium` at 6 could no longer preempt it. The kernel restored `low`'s
own priority at the unlock, which is why `medium` then runs and `low` finishes last.
*(1.5 marks)*

**What the mutex does not fix.** It does not remove the wait: `high` still waits for the whole of
`low`'s critical section, and keeping that short is still the designer's job. Inheritance bounds the
wait by the critical section, it does not eliminate it. Credit equally: it does nothing for a
semaphore used as a lock, it does not prevent deadlock, and it cannot help a lock held across a
blocking call.
*(0.5 marks)*

### (b) 3 marks

**What `-ENOMSG` means here.** The queue is full and the call was told not to wait, so nothing was
queued: the sample is dropped. The producer is being told that the consumer is not keeping up.
*(1 mark)*

**What `K_FOREVER` would do.** Block the producer until a slot appears. Nothing is lost, and the
producer is throttled to the consumer's rate — which is right for a pipeline and wrong for anything
that must keep sampling on time.
*(1 mark)*

**Which an interrupt handler may use.** Only `K_NO_WAIT`. An ISR may not block, so the only choice
in interrupt context is to accept that a full queue means a dropped message — and, ideally, to count
the drops.
*(1 mark)*

### (c) 4 marks

**The subscriber's behaviour.** A subscriber is notified through a queue of channel *references*,
not of copies: it is told which channel changed, and reads the channel's current value when its
thread eventually runs. If two messages are published before it runs, both notifications are
waiting, and both reads return the second message — so it prints the newer reading twice and never
sees the one that was overwritten. That is not a bug: for something like a display, only the latest
value matters. A message subscriber is queued a *copy* of each message, so it sees every one exactly
once, at the cost of the buffer the copy lives in.
*(2.5 marks)*

**Where a listener runs.** Inside `zbus_chan_pub()`, in the **publisher's** thread, with the
channel's lock held. It therefore must not block, must not sleep, and must not publish to the same
channel; it should copy or flag and return. That is also why a listener never misses a message: it
is called before the publish returns.
*(1.5 marks)*

---

## Question 7 - Writing a driver (13 marks)

### (a) 4 marks

**What `device_is_ready()` reports on.** Two things: that the device exists in this build and was
initialized by the kernel, and that its init function reported success. It is a query of the
device's initialization state, not a probe of the hardware.
*(1.5 marks)*

**What the driver most likely did.** Its init function returned a negative errno — the usual cause
being the identity check: the driver read the part's `WHO_AM_I` register over I2C and got something
other than the expected value, and returned `-ENODEV`. The kernel marks the device not ready and
carries on booting.
*(1.5 marks)*

**If the application used it anyway.** It would call into a driver whose state was never set up:
`dev->data` holds whatever init left, the bus may never have been configured, and the reads return
stale or nonsense values, or fault. The whole point of returning the error at init is that the check
is the one place to catch it, so a user that skips the check turns a clean "not ready" into a
misbehaving system.
*(1 mark)*

### (b) 5 marks

1. **The devicetree values are hard-coded** (`DEVICE_DT_GET(DT_NODELABEL(i2c0))` and `0x48`) rather
   than taken from the instance being defined. The second node in the tree gets the first one's bus
   and address, so two instances talk to the same chip, and the driver silently ignores what the
   devicetree says. The corrected shape takes them from the instance:

   ```c
   .i2c = I2C_DT_SPEC_INST_GET(inst),
   ```

   filled in by the instance macro that defines each device. *(2 marks)*

2. **The `i2c_dt_spec` lives in `dev->data`.** It never changes, so it belongs in a `const` config
   struct reached through `dev->config`, which on a real target lives in flash. As written, every
   instance spends RAM on constants, and nothing stops the driver from modifying what the devicetree
   said:

   ```c
   struct thermo_config {
   	struct i2c_dt_spec i2c;
   };
   ```

   with `dev->data` left for the part that does change, such as `last_sample`. *(1.5 marks)*

3. **The identity read's result is discarded and init returns 0 regardless.** `(void)` on
   `i2c_reg_read_byte_dt()` throws away the bus error, and `id` is never compared with anything, so
   a missing, broken or wrong part reports itself ready and the application trusts it. Init must
   check both:

   ```c
   ret = i2c_reg_read_byte_dt(&config->i2c, THERMO_REG_WHO_AM_I, &id);
   if (ret < 0) {
   	return ret;
   }
   if (id != THERMO_WHO_AM_I) {
   	return -ENODEV;
   }
   ```

   *(1.5 marks)*

Credit also, without extra marks, a candidate who notes that init should check the bus itself with
`i2c_is_ready_dt(&config->i2c)` before using it, that `DT_DRV_COMPAT` is only useful with the
`DT_INST_*` macros and `DT_INST_FOREACH_STATUS_OKAY()`, and that nothing here would define more than
one device in the first place.

### (c) 2 marks

**Where the symbol comes from.** The build generates one `DT_HAS_<COMPATIBLE>_ENABLED` symbol per
compatible that appears on an enabled node in the final devicetree, and Kconfig can depend on it
because Kconfig runs after devicetree.
*(1 mark)*

**What it achieves, and what disabling the node does.** The driver's option can only be on when the
hardware is in the tree, so a build that has no such part cannot accidentally include the driver,
and one that does gets it by default. With `status = "disabled"`, the symbol is not generated, the
option goes to `n`, the driver is not compiled — and the application's `DEVICE_DT_GET()` fails at
compile time with the `__device_dts_ord_N` undeclared error of Question 3.
*(1 mark)*

### (d) 2 marks

**What the numbers order.** Initialization within the same level: at `POST_KERNEL`, every device
with priority 50 is initialized before every device with priority 90. The pair exists so that a
device is initialized after whatever it depends on.
*(1 mark)*

**If the sensor were 40.** The sensor's init would run before the I2C controller's, so its identity
read would go through a bus that is not yet initialized. It would fail, init would return an error,
and the device would be not ready for the rest of the run — a boot-order bug that looks exactly like
broken hardware.
*(1 mark)*

---

## Question 8 - Subsystems and debugging (12 marks)

### (a) 3 marks

* **`-22` is `-EINVAL`, returned by the command's own handler.** The handler validated its argument,
  printed why it was refused, and returned the errno.
* **`-8` is `-ENOEXEC`, returned by the shell itself** for a command it does not know.
* **`1` is `SHELL_CMD_HELP_PRINTED`**, returned when a command with subcommands is run without one:
  all it did was print its help.

*(1.5 marks, half each.)*

**What the shell does with the value.** It does not print it, but it keeps it, and the built-in
`retval` command prints the previous command's return value.
*(0.5 marks)*

**Why return a negative errno.** Because the value is what a script, a test, or anything driving the
shell can act on: a handler that prints an error and returns 0 reports success to everything except
a human reading the screen. Returning the errno makes the failure machine-readable and keeps the
shell's own conventions intact.
*(1 mark)*

### (b) 3 marks

**Where the counter lives.** In the settings subsystem's storage, on the board's flash — and on
`native_sim` that flash is a file, `flash.bin`, in the directory the program runs in. `west build -t
run` runs the program in the build directory, so the file is `build/flash.bin`.
*(1.5 marks)*

**Why a pristine build resets it.** `west build -p` deletes the build directory and everything in
it, `flash.bin` included. The next run therefore starts with erased flash — a factory-new device —
and the counter starts again at 1.
*(1 mark)*

**Making every run a first boot without deleting anything.** `-flash_in_ram`, which keeps the
simulated flash in RAM only and leaves the file alone. Credit `-flash_erase`, which erases at
startup, as an answer that also achieves it; credit `-flash=<path>` only if the candidate explains
that it selects a different file.
*(0.5 marks)*

### (c) 3 marks

**The difference.** Reason 3 is a **kernel oops**: an error attributed to the thread that caused it,
raised by `k_oops()` or an equivalent. Reason 4 is a **kernel panic**, which a failed `__ASSERT()`
raises: an error the system as a whole cannot continue from.
*(1 mark)*

**What the handler must do.** For the oops it may **return**, and the kernel then aborts only the
offending thread — which is what the first trace shows, `main()` carrying on afterwards. For the
panic it must **not** return; it has to halt or reboot the system, because there is no defined state
to go back to.
*(1 mark)*

**The exit status.** On `native_sim`, halting means the process exits with **status 1**, which is
also why a test that ends this way is reported as a failure rather than as output.
*(1 mark)*

### (d) 3 marks

**What the report tells you.** The kind of error (a write past the end of a stack buffer), the
address, the thread, and the exact source line that did it — with a frame pointing at `fill()` in
`main.c:28`. On a microcontroller the same write would simply have landed on whatever variable was
next in memory, and the symptom would have appeared somewhere else entirely, later, as corrupted
data.
*(1.5 marks)*

**Why it stops.** AddressSanitizer halts the program at the first error rather than letting it
continue on memory it has proved is wrong; the process exits with status 1.
*(0.5 marks)*

**Why only on this board.** AddressSanitizer is the host toolchain's: it instruments the program and
links it against the host's runtime, which exists because a `native_sim` image is an ordinary Linux
program. There is no such runtime in a Cortex-M image, and `CONFIG_ASAN` depends on the POSIX
architecture accordingly. Credit a candidate who adds that UndefinedBehaviorSanitizer is not in the
same position — it can trap on a target through `CONFIG_UBSAN_TRAP` — and that this is a large part
of why it pays to keep code runnable on `native_sim`.
*(1 mark)*

---
