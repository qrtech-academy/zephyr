# Shell
Shell commands of your own: `led on|off|status`, a command with subcommands, backed by the emulated
LED, and `greet <name> [times]`, a command with arguments. The shell also brings the kernel's
commands and, with `CONFIG_LOG_CMDS`, the `log` command, so a running program can be inspected and
its logging changed without rebuilding it.

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/shell
west build -t run
```

Type `help`, press `Tab` twice, try the commands below, and stop the program with `Ctrl+C`.

```text
uart:~$ greet Ada 2
Hello, Ada!
Hello, Ada!
uart:~$ led on
LED on
uart:~$ led status
LED is on
```

Commands can also be piped in, which is how you script a session:

```bash
printf 'greet Ada\nled on\nled status\nkernel uptime\n' | ./build/zephyr/zephyr.exe -stop_at=1
```

---

## What to look at
* **`CONFIG_UART_NATIVE_PTY_0_ON_STDINOUT=y`** in [`prj.conf`](./prj.conf) puts the shell on the
  terminal you started the program from. Without it, `native_sim` gives the UART a pseudo-terminal
  of its own and prints `uart connected to pseudotty: /dev/pts/11` (the number varies), which you
  connect to from a second terminal, e.g. with `screen /dev/pts/11`. The command-line option
  `-uart_stdinout` does the same as the Kconfig option without rebuilding.
* **The shell checks the argument count for you.** `greet` alone prints
  `greet: wrong parameter count` and the command's help, because `SHELL_CMD_ARG_REGISTER` declared
  2 mandatory words and 1 optional. The handler only checks what the words say: `greet Ada 9`
  prints `times must be between 1 and 5, not '9'` in red and returns `-EINVAL`.
* **`kernel thread list`** shows the threads that exist: the shell's own `shell_uart` thread and
  `idle`. `main` is not there, because `main()` has returned. Its stack figures, such as
  `usage 40 / 3072 (1 %)`, mean nothing on `native_sim`: threads run on host pthread stacks, and
  the Zephyr stack only holds a little bookkeeping. On a board the same command is how you size
  stacks.
* **`log status`** lists every log module with its current and built-in level. `log enable dbg app`
  answers `app: level set to inf.`: a module's level can be lowered at run time, but never raised
  above the level it was compiled with (`LOG_LEVEL_INF` in `main.c`). `log disable app` silences it.
* **[`sample.yaml`](./sample.yaml)** uses Twister's `shell` harness, which types each command into the
  running program and matches a line of the response against a regular expression.

---
