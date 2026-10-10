# Appendix A
## Setting Up the Workspace
Every program in this course is built and run on Linux. This appendix installs what the workspace
needs, sets it up, and checks that it works. Do it before the first lecture.

---

## Linux
Any recent x86-64 Linux works. On Windows, use **WSL 2** with **Ubuntu 24.04**, as in the other
courses in this track. In **Windows PowerShell as Administrator**:

```bash
wsl --install -d Ubuntu-24.04
```

Restart if asked, start *Ubuntu* from the Start menu, and choose a user name and password.
`native_sim` needs WSL **2**: the old WSL 1 cannot run the programs it builds. Check with
`wsl --status` in PowerShell, which must say `Default Version: 2`.

**Keep the repository in the Linux file system**, for example in `~/qacademy`, and not under
`/mnt/c`. A Zephyr build touches thousands of files, and through `/mnt/c` it is many times slower.

---

## Packages
In the Ubuntu terminal:

```bash
sudo apt -y update
sudo apt -y install git make cmake gcc g++ python3 python3-venv python3-pip clang-format gdb
```

What each one is for:

| Package | Used for |
|---------|----------|
| `git` | Fetching the course repository, and Zephyr through west. |
| `make` | The course's `make setup`, `make test` and friends. |
| `cmake` | Zephyr's build system. Version 3.20 or newer; Ubuntu 22.04 and 24.04 both have it. |
| `gcc`, `g++` | The compiler. `native_sim` programs are Linux programs, built by the host's own GCC. |
| `python3`, `python3-venv`, `python3-pip` | west, Twister, and the scripts that process Kconfig and devicetree. |
| `clang-format` | `make format`, which formats C files the way Zephyr does. |
| `gdb` | Debugging in L06. |

**No Zephyr SDK is needed.** The SDK holds cross-compilers for real microcontrollers; `native_sim`
uses the host compiler, which `source env.sh` (below) tells Zephyr to do.

Two packages are optional:
* `device-tree-compiler` (`dtc`). Zephyr processes devicetree with its own Python scripts and uses
  `dtc` only for extra warnings. Without it, every build prints
  `Could NOT find Dtc (missing: DTC)`, which is harmless.
* `gcc-multilib g++-multilib`. The course builds for `native_sim/native/64`, a 64-bit program. The
  plain `native_sim` board is 32-bit, and building for it without these packages fails with
  `fatal error: bits/libc-header-start.h: No such file or directory`.

---

## The workspace
Clone the repository and set it up:

```bash
git clone https://github.com/qrtech-academy/zephyr.git
cd zephyr
make setup
```

`make setup` does four things, all inside the repository:
1. Creates a Python environment in `.venv` and installs west into it.
2. Makes the repository a west workspace, with [`west.yml`](../../../west.yml) as its manifest.
3. Runs `west update`, which fetches the Zephyr release that `west.yml` pins into `deps/zephyr`.
4. Installs Zephyr's Python requirements, Twister's among them, into `.venv`.

It takes a few minutes and about 1 GB of disk. Nothing is installed outside the repository, and
running it again is safe.

Then, **in every new terminal**:

```bash
source env.sh
```

That activates the Python environment, so that `west` is found, and sets two variables:
`ZEPHYR_BASE`, which says where Zephyr is, and `ZEPHYR_TOOLCHAIN_VARIANT=host`, which says to build
with the host's GCC. Your prompt starts with `(.venv)` while it is active.

---

## Checking that it works
From the repository root:

```bash
west build -p -b native_sim/native/64 lectures/L01/examples/hello
west build -t run
```

The first command configures and builds the example into `build/`, which takes about half a minute
the first time. The second runs it:

```text
*** Booting Zephyr OS build v4.4.2 ***
Hello from native_sim/native/64
[00:00:00.000,000] <inf> hello: Tick 1 at 0 ms
[00:00:00.510,000] <inf> hello: Tick 2 at 510 ms
[00:00:01.020,000] <inf> hello: Tick 3 at 1020 ms
[00:00:01.530,000] <wrn> hello: A warning, in yellow
```

The program keeps running after that, because a Zephyr system does not exit when `main()` returns.
Stop it with `Ctrl+C`. Finally, check that the test suites run:

```bash
make test
```

With no exercises written yet, every exercise set is reported as `SKIP`, and the command succeeds.

---

## Visual Studio Code
With the **WSL** extension, run `code .` from the repository root in the Ubuntu terminal. For code
navigation that understands Zephyr's include paths and `CONFIG_` macros, install the **C/C++**
extension and point it at the build's compilation database, which every build writes to
`build/compile_commands.json`. In `.vscode/settings.json`:

```json
{
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

---

## When something goes wrong

| Message | Cause |
|---------|-------|
| `west: command not found` | `source env.sh` has not been run in this terminal. |
| `ZEPHYR_TOOLCHAIN_VARIANT not set, trying to locate Zephyr SDK` followed by `Could not find a package configuration file provided by "Zephyr-sdk"` | Also `source env.sh`: without it, Zephyr looks for the SDK. |
| `error: the workspace is not set up` | `make setup` has not been run, or did not finish. Run it again. |
| `fatal error: bits/libc-header-start.h: No such file or directory` | Building for the 32-bit `native_sim` without `gcc-multilib`. Use `native_sim/native/64`. |
| A build that fails for no reason you can see after changing boards or options | A stale build directory. Build again with `-p` (pristine). |

---
