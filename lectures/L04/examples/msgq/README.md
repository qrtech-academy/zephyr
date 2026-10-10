# Message Queue
A producer makes a sample every 100 ms, and a consumer takes 250 ms to process each one. A
`k_msgq` of four samples sits between them. The producer runs twice: first it never waits, so
samples are dropped when the queue is full, and then it waits forever, so nothing is dropped and the
producer slows down to the consumer's rate.

```bash
west build -p -b native_sim/native/64 lectures/L04/examples/msgq
./build/zephyr/zephyr.exe -stop_at=6 -no-rt
```

---

## What to look at
* **`consumed #1` is printed before `produced #1`.** The consumer has the higher priority and is
  already waiting, so `k_msgq_put()` hands it the sample and switches to it before `put` returns.
  That is also why the queue reads `0/4` right after the first put.
* **With `K_NO_WAIT`, samples #8, #10 and #12 are dropped**, and the ones that get through are
  almost a second old by the time they are consumed. The producer keeps its rate and never blocks,
  which is the only option in an interrupt handler.
* **With `K_FOREVER`, nothing is dropped**, but from #21 on every put blocks for 150 ms. The full
  queue slows the producer down to the consumer's 250 ms per sample.
* **The periods are 101 and 251 ms**, not 100 and 250. `k_msleep()` sleeps for at least the time
  asked for and ends on the tick after it, and `prj.conf` sets one tick per millisecond.
* **The message is copied** into the queue and back out, so the producer's local `struct sample`
  can go out of scope as soon as the put returns. Keep messages small, or queue pointers instead.

---
