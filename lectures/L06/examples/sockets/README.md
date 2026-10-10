# Sockets
A TCP echo server written against Zephyr's socket API (`zsock_socket()`, `zsock_accept()`,
`zsock_recv()`, ...). On `native_sim`, the *offloaded sockets* driver turns every one of those calls
into the host's own socket call. The server then listens on a real port of your Linux machine, and
any host program can connect to it, with no TAP interface, no bridge and no root privileges.

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/sockets
west build -t run
```

In a second terminal, connect with netcat, type a line, and it comes back:

```bash
nc 127.0.0.1 4242
```

```text
[00:00:00.000,000] <inf> app: Listening on TCP port 4242
[00:00:00.970,001] <inf> app: Client connected
[00:00:00.970,003] <inf> app: Echoed 20 bytes
[00:00:00.970,004] <inf> app: Client disconnected
```

With [`self_test.conf`](./self_test.conf), a client thread in the same program connects to the
server, sends a message and checks the echo. That is how Twister tests the example:

```bash
west build -p -b native_sim/native/64 lectures/L06/examples/sockets -- -DEXTRA_CONF_FILE=self_test.conf
west build -t run
```

---

## What to look at
* **What is being tested is the application, not a network stack.** With
  `CONFIG_NET_NATIVE_OFFLOADED_SOCKETS`, Zephyr's TCP/IP stack is not in the program at all; Linux
  does the TCP. That is exactly what you want when developing the protocol on top, and exactly why
  a board, with Zephyr's own stack and a real network interface, still has to be tested.
* **Blocking calls block one thread, not the program.** `zsock_accept()` and `zsock_recv()` wait
  inside the kernel, like `k_sem_take()`, and the other threads run meanwhile. The driver polls the
  host sockets once every simulated millisecond (`CONFIG_NET_NATIVE_OFFLOADED_SOCKETS_EPOLL_WAIT_INTERVAL`).
* **The workaround in `main.c` after `zsock_accept()`** sets the new socket's receive and send
  timeouts. In Zephyr 4.4.2 the offloaded-sockets driver leaves them uninitialized for accepted
  sockets. Remove the two `zsock_setsockopt()` calls, and the program freezes at `Client connected`:
  `recv()` goes straight to the host and blocks the whole process, simulated time and all. The
  simulator is software too, with bugs of its own.
* **`WARNING: Using a test - not safe - entropy source`** is printed because networking asks for
  random numbers, and `native_sim`'s entropy device is a seeded generator, deterministic by
  design. `-seed=<n>` picks the seed; `-seed-random` seeds it from `/dev/urandom`.
* **The port is an ordinary Kconfig option**, `CONFIG_APP_PORT` in [`Kconfig`](./Kconfig). Only one
  program can listen on a port at a time: a second copy logs `Cannot listen on port 4242: 98`,
  where 98 is `EADDRINUSE`.

---
