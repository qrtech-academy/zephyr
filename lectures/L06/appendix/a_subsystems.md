# Appendix A
## Subsystems: the Shell, Settings and Networking
L01 to L05 built programs that print. This appendix covers three subsystems that make a program
part of a system: a shell to talk to it, settings to make it remember, and sockets to put it on a
network. All three are the same code on a board and on `native_sim`; what `native_sim` puts under
them, a terminal, a file and the host's network, is what this appendix is careful to separate
from the subsystems themselves. By the end you should be able to explain every file in the
[`shell`](../examples/shell), [`settings`](../examples/settings) and
[`sockets`](../examples/sockets) examples.

---

## The shell
### What it is
Zephyr's shell is a command line inside the firmware: line editing, history, `Tab` completion, a
`help` for every command, and colors. Subsystems register commands of their own, so enabling the
shell alone already gives you commands for the kernel, and enabling logging or settings with it
gives you commands for those. An application adds its own the same way.

```text
CONFIG_SHELL=y
```

The shell runs in a thread of its own, and reads and writes through a **backend**. The usual one is
the serial backend, on the UART that the devicetree's `chosen` node names `zephyr,shell-uart`;
`native_sim`'s devicetree names `uart0`. Others carry the same shell over SEGGER RTT, telnet, a
WebSocket, MQTT or RPMsg, and a *dummy* backend runs commands from code, which is how the shell is
tested. The thread of the serial backend is called `shell_uart`.

### The shell on native_sim
`native_sim`'s `uart0` is a pseudo-terminal by default. The program prints
`uart connected to pseudotty: /dev/pts/11` (the number varies), and you connect to that from a
second terminal. For a course, and for tests, it is simpler to put the UART on the terminal the
program was started from:

```text
CONFIG_UART_NATIVE_PTY_0_ON_STDINOUT=y
```

or, without rebuilding, the command-line option `-uart_stdinout`. Commands can then be typed, or
piped in, which is how a session is scripted:

```bash
printf 'greet Ada\nled on\nled status\n' | ./build/zephyr/zephyr.exe -stop_at=1
```

The shell echoes each command after its prompt, `uart:~$`, and prints the response on the lines
that follow.

### Commands
A command is a handler function and a static entry that describes it. Every handler has the same
signature:

```c
static int cmd_greet(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "Hello, %s!", argv[1]);
	return 0;
}
```

* **`sh`** is the shell instance that ran the command, and the one its output must go to: there may
  be several backends, each with a shell of its own.
* **`argv`** is the command line split into words, **`argv[0]`** being the command's own name, and
  **`argc`** is how many there are.
* **The return value** is `0` on success and a negative `errno` on failure.

A command with no subcommands is registered with its argument counts:

```c
SHELL_CMD_ARG_REGISTER(greet, NULL, "Greet someone: greet <name> [times]", cmd_greet, 2, 1);
```

The last two numbers are the **mandatory** and **optional** word counts, and the mandatory count
**includes the command's own name**: `2, 1` means `greet` and a name, and optionally one more
word. The shell checks the count before it calls the handler, so the handler never sees `argc`
outside the range. A wrong count prints the command's help instead:

```text
uart:~$ greet
greet: wrong parameter count
greet - Greet someone: greet <name> [times]
```

`SHELL_CMD_REGISTER()` is the same without counts, for a command that takes no arguments or checks
them itself. Commands with **subcommands** have a table of them, built at compile time:

```c
SHELL_STATIC_SUBCMD_SET_CREATE(sub_led,
	SHELL_CMD(on, NULL, "Turn the LED on.", cmd_led_on),
	SHELL_CMD(off, NULL, "Turn the LED off.", cmd_led_off),
	SHELL_CMD_ARG(blink, NULL, "Blink: blink <times>", cmd_led_blink, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(led, &sub_led, "Control the LED.", NULL);
```

A subcommand is written with `SHELL_CMD()` or, with counts, `SHELL_CMD_ARG()`, and its counts again
include its own name: in `led blink 3`, the handler for `blink` gets `argv[0] = "blink"` and
`argv[1] = "3"`. A command with subcommands and no handler of its own, `NULL` above, prints its help
and the list of subcommands when it is run alone. Because the tables are static, `Tab` can complete
them. (`SHELL_DYNAMIC_CMD_CREATE()` creates subcommands at run time, such as a list of device
names, and `SHELL_COND_CMD_REGISTER()` registers a command only when a Kconfig option is enabled.)

### Output
A handler prints through the shell it was called by, never with `printk()`, whose output would go
to the console and not necessarily to the backend the user is on:

| Function | Output |
|----------|--------|
| `shell_print(sh, fmt, ...)` | Normal output. |
| `shell_info(sh, fmt, ...)` | Informational output, in green. |
| `shell_warn(sh, fmt, ...)` | A warning, in yellow. |
| `shell_error(sh, fmt, ...)` | An error, in red. |
| `shell_hexdump(sh, data, len)` | A buffer, as a hex dump. |

The functions add the newline themselves.

### Validating arguments
The shell counts words; what the words say is the handler's business. `atoi()` is the wrong tool,
because it cannot fail: `atoi("warm")` is 0 and `atoi("25x")` is 25. `strtol()` reports where it
stopped and whether the value overflowed, so a word is a number exactly when it stopped at the end
of the word, after at least one digit, with `errno` still 0:

```c
char *end;

errno = 0;
long value = strtol(argv[1], &end, 10);

if ((end == argv[1]) || (*end != '\0') || (errno != 0)) {
	shell_error(sh, "not a temperature: '%s'", argv[1]);
	return -EINVAL;
}
```

A value that is a number but out of range is a different error, conventionally `-ERANGE`.

### Return values and `retval`
The shell does not print a handler's return value, but it keeps it: the built-in command `retval`
prints the value the previous command returned. That makes errors visible to a script, and makes
the shell's own conventions visible to you:

```text
uart:~$ greet Ada 9
times must be between 1 and 5, not '9'
uart:~$ retval
-22
uart:~$ foo
foo: command not found
uart:~$ retval
-8
```

`-22` is `-EINVAL`, returned by the handler; `-8` is `-ENOEXEC`, returned by the shell for a command
that does not exist. A command with subcommands run without one returns `1`
(`SHELL_CMD_HELP_PRINTED`), since all it did was print its help.

### The built-in commands
With the shell come commands from the subsystems that are enabled. In Zephyr 4.4:

| Command | From | What it does |
|---------|------|--------------|
| `help`, `history`, `clear`, `retval` | The shell itself, `help` always and the others by default | The shell's own commands. |
| `kernel uptime`, `kernel version` | The kernel shell (`CONFIG_KERNEL_SHELL`, on with the shell) | The system's uptime and Zephyr's version. |
| `kernel thread list`, `kernel thread stacks` | The kernel shell | Every thread, its priority, state and stack usage. Zephyr 3.7 called them `kernel threads` and `kernel stacks`. |
| `log status`, `log enable <level> <module>`, `log disable <module>` | Logging, with `CONFIG_LOG_CMDS=y` | Every log module's level, changed at run time. |
| `settings list`, `settings read <name>`, `settings write`, `settings delete` | Settings, with `CONFIG_SETTINGS_SHELL=y` | The stored settings. |

Two of them deserve a warning on `native_sim`:
* **`kernel thread list`** prints stack usage such as `usage 40 / 3072 (1 %)`, which means nothing
  here: L03 showed that threads run on host stacks. On a board it is how stacks are sized.
* **`log enable dbg app`** answers `app: level set to inf.` for a module compiled at `LOG_LEVEL_INF`.
  A level can be lowered at run time, never raised above what was compiled in, because the
  messages above it are not in the program at all (L01).

---

## Settings
### What it is
The settings subsystem stores **key-value pairs** in non-volatile memory, so that a device remembers
them across a restart: a calibration, a network name, a setpoint. The keys are names made of parts
separated by `/`, such as `thermostat/setpoint`, and the first part is a **subtree** that one module
owns. The values are bytes, of whatever size and type the owner decides.

```text
CONFIG_SETTINGS=y
```

The subsystem does not write to memory itself. It hands the pairs to a **backend**, chosen with a
Kconfig `choice`. Its defaults include, in order:

| Backend | Chosen by default when |
|---------|------------------------|
| ZMS, Zephyr Memory Storage | `CONFIG_ZMS=y` |
| NVS, Non-Volatile Storage | `CONFIG_NVS=y` |
| FCB, Flash Circular Buffer | `CONFIG_FCB=y` |
| A file | `CONFIG_FILE_SYSTEM=y` |
| None | nothing else is enabled: settings then live in RAM only |

ZMS and NVS are both log-structured key-value stores for flash: they append new values and erase
whole sectors when the old values are no longer needed, so that no single flash cell wears out
early. ZMS is the newer of the two, and is designed for memories that need no erase as well. The
course uses ZMS:

```text
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_ZMS=y
CONFIG_SETTINGS=y
```

**Where on the flash** is a devicetree question. The ZMS backend uses the partition that the
`chosen` node names `zephyr,settings-partition`, or, when there is none, the partition labelled
`storage_partition`. `native_sim`'s devicetree has one: 16 KiB at offset `0xfc000` of its 2 MiB flash.

### Handlers
A module that owns a subtree registers a **handler** for it: a set of callbacks, any of which may be
`NULL`.

| Callback | Called by | To |
|----------|-----------|----|
| `h_set(name, len, read_cb, cb_arg)` | `settings_load()`, once for every stored setting in the subtree | Take the stored value into the module's variables. |
| `h_commit()` | `settings_load()`, after every `h_set` | Act on the whole set of values at once. |
| `h_export(export_func)` | `settings_save()` | Hand every value of the subtree over for saving. |
| `h_get(name, val, len)` | `settings_runtime_get()` | Return the value the module holds in RAM. |

The handler is defined at compile time, like a shell command:

```c
SETTINGS_STATIC_HANDLER_DEFINE(thermostat, "thermostat", NULL, thermostat_set, NULL, NULL);
```

`h_set` gets the rest of the name, after the subtree, and **not the value itself**. The subsystem
does not know how big the value should be, so the handler checks the stored length and reads the
value into storage of its own:

```c
static int thermostat_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;

	if (!settings_name_steq(name, "setpoint", &next) || (next != NULL)) {
		return -ENOENT;
	}
	if (len != sizeof(setpoint_c)) {
		return -EINVAL;
	}

	ssize_t read = read_cb(cb_arg, &setpoint_c, sizeof(setpoint_c));

	return (read < 0) ? (int)read : 0;
}
```

`settings_name_steq(name, "setpoint", &next)` is true when `name` begins with `setpoint`, and sets
`next` to what follows the next `/`, or to `NULL` if nothing does. The length check is not
pedantry: flash written by an older firmware, with a smaller type, is still there after an update.

### Loading, saving and deleting

```c
settings_subsys_init();                       /* Mount the backend. */
settings_load();                              /* Call h_set for everything stored, then h_commit. */
settings_save_one("thermostat/setpoint", &value, sizeof(value));
settings_delete("thermostat/setpoint");
```

**Loading happens once, at boot, and on the application's initiative.** Until `settings_load()`
runs, every variable holds its initial value, which is also what it keeps if nothing was ever
stored: on the very first boot, no handler is called at all. Saving writes one value at once; there
is no transaction around several.

### The flash on native_sim
`native_sim`'s flash is simulated in a file, **`flash.bin`, in the directory the program runs in**:

```bash
west build -t run          # Runs in build/, so the file is build/flash.bin.
./build/zephyr/zephyr.exe  # Runs where you are, and uses a flash.bin there.
```

The file is the whole 2 MiB of the simulated flash. A pristine build, `west build -p`, deletes
`build/` and the flash with it: a factory-new device. The command-line options decide what happens
to the flash for one run:

| Option | Effect |
|--------|--------|
| `-flash=<path>` | Use another file. |
| `-flash_erase` | Erase the flash at startup. |
| `-flash_rm` | Delete the file when the program ends. |
| `-flash_in_ram` | Keep the flash in RAM only. Every run starts erased, and the file is left alone. |

`strings build/flash.bin` shows the names of the stored settings, `boot/count` for the
[`settings`](../examples/settings) example, because the backend stores each name next to its value.
With `CONFIG_SETTINGS_SHELL=y`, the shell shows the same from inside:

```text
uart:~$ settings list
thermostat/setpoint
uart:~$ settings read thermostat/setpoint
00000000: 1b 00 00 00                                      |....             |
```

`1b 00 00 00` is 27, as a little-endian `int32_t`: the bytes exactly as the handler wrote them.

---

## Networking on native_sim
### Two ways onto a network
Zephyr has its own TCP/IP stack, and on a board that is what runs under the socket API. `native_sim`
can run it too, over a TAP interface on the host (`CONFIG_ETH_NATIVE_TAP`), but that needs the TAP
interface set up on the host, with root privileges.

The alternative is **offloaded sockets**:

```text
CONFIG_NETWORKING=y
CONFIG_NET_IPV4=y
CONFIG_NET_SOCKETS=y
CONFIG_NET_DRIVERS=y
CONFIG_NET_SOCKETS_OFFLOAD=y
CONFIG_NET_NATIVE_OFFLOADED_SOCKETS=y
```

With them, every socket call is handed to the host's own socket API. A server listens on a real port
of your machine, a client connects to real addresses, and nothing needs setting up. The price is in
the name: Zephyr's TCP/IP stack is not in the program at all, and Linux does the networking. That is
what you want while developing the protocol on top, and it is exactly why the same program still has
to be tested on a board.

### The socket API
Zephyr's socket API is BSD sockets with a prefix:

| BSD | Zephyr |
|-----|--------|
| `socket()`, `bind()`, `listen()`, `accept()`, `connect()` | `zsock_socket()`, `zsock_bind()`, `zsock_listen()`, `zsock_accept()`, `zsock_connect()` |
| `send()`, `recv()`, `sendto()`, `recvfrom()`, `close()` | `zsock_send()`, `zsock_recv()`, `zsock_sendto()`, `zsock_recvfrom()`, `zsock_close()` |
| `setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, ...)` | `zsock_setsockopt(s, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, ...)` |
| `struct sockaddr_in`, `AF_INET`, `htons()` | `struct net_sockaddr_in`, `NET_AF_INET`, `net_htons()` |

**Zephyr 4.4 prefixed the types and constants too.** Before it, `struct sockaddr_in` and `AF_INET`
were Zephyr's own names, which collided with the C library's and POSIX's. Now the networking headers
define `net_`, `NET_` and `ZSOCK_` names, and a compatibility header, enabled by
`CONFIG_NET_NAMESPACE_COMPAT_MODE` (on by default), keeps the old names working. New code should
use the prefixed ones, as the examples do; code that wants the plain POSIX names includes the POSIX
headers and enables Zephyr's POSIX API.

### Blocking
A blocking call blocks **the calling thread**, not the program: `zsock_recv()` waits inside the
kernel, like `k_sem_take()`, and every other thread runs meanwhile. Underneath, the offloaded
sockets driver asks the host which sockets are ready once every simulated millisecond
(`CONFIG_NET_NATIVE_OFFLOADED_SOCKETS_EPOLL_WAIT_INTERVAL`).

That holds only while the driver knows the socket's timeouts. **In Zephyr 4.4.2, the driver allocates
the socket that `zsock_accept()` returns without initializing its timeouts**, and a blocking
`zsock_recv()` on it can go straight to the host's `recv()`, which blocks the whole process,
simulated time and every thread with it. The
[`sockets`](../examples/sockets) example froze at `Client connected` until it set both timeouts
explicitly after accepting:

```c
struct zsock_timeval forever = {0};

zsock_setsockopt(client, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &forever, sizeof(forever));
zsock_setsockopt(client, ZSOCK_SOL_SOCKET, ZSOCK_SO_SNDTIMEO, &forever, sizeof(forever));
```

A timeout of zero means *wait forever*, as with BSD sockets. Sockets created with `zsock_socket()`
are not affected: the driver sets their timeouts when it creates them. The simulator is software,
with bugs of its own.

### TCP and UDP
The [`sockets`](../examples/sockets) example is a TCP server: `listen()`, `accept()` a connection,
then `recv()` and `send()` a stream of bytes, with no message boundaries, until the client closes
it. UDP, which Exercise Set 5 uses, has no connections: a server `bind()`s a port and answers each
datagram, whole, to the address `recvfrom()` says it came from. Nothing is retransmitted, so a
client that waits for an answer should wait with a timeout (`ZSOCK_SO_RCVTIMEO`), not forever.

A program using offloaded sockets can be tested from the host with netcat:

```bash
nc 127.0.0.1 4242       # TCP, the sockets example.
nc -u 127.0.0.1 4300    # UDP, Exercise Set 5.
```

On `native_sim`, networking also prints `WARNING: Using a test - not safe - entropy source` at boot:
the network stack wants random numbers, and `native_sim`'s entropy device is a seeded generator,
deterministic by design.

---

## Summary
* The shell is a command line inside the firmware, running in a thread of its own over a backend.
  On `native_sim`, `CONFIG_UART_NATIVE_PTY_0_ON_STDINOUT` puts it on the terminal.
* A command is a handler and a static entry. The mandatory word count includes the command's name,
  and the shell checks the count; the handler checks what the words say, with `strtol()`, not
  `atoi()`.
* Handlers print with `shell_print()` and `shell_error()`, and return `0` or a negative `errno`,
  which `retval` shows.
* Settings are named key-value pairs, owned by subtree handlers, stored by a backend such as ZMS in
  the flash partition the devicetree names.
* `settings_load()` calls each handler's `h_set` with the stored length and a function to read the
  value; nothing is loaded until the application asks.
* `native_sim`'s flash is the file `flash.bin` in the working directory, controlled by `-flash`,
  `-flash_erase`, `-flash_rm` and `-flash_in_ram`.
* Offloaded sockets put the host's network under Zephyr's socket API, without setup and without
  Zephyr's IP stack. Zephyr 4.4 prefixes the socket API's names with `zsock_`, `net_` and `NET_`.
* Blocking socket calls block one thread, as long as the socket's timeouts are set: set them
  explicitly on accepted sockets in Zephyr 4.4.2.

---
