# State Machine Framework
A heater controller written with Zephyr's State Machine Framework (SMF). IDLE, HEATING and HOLD are
children of an OPERATIONAL parent state, which handles a fault in any of them. FAULT is outside
OPERATIONAL, and only a reset leaves it. A second thread plays the part of the rest of the system and
posts events to a `k_event`. The state machine's thread waits on that `k_event` and runs the state
machine once for each event.

```bash
west build -p -b native_sim/native/64 lectures/L04/examples/smf
./build/zephyr/zephyr.exe -stop_at=2 -no-rt
```

---

## What to look at
* **The state machine is a table.** Each state in `states[]` has an entry, a run and an exit
  action, and a parent. `smf_set_state()` runs the exit actions of the states being left and the
  entry actions of the states being entered.
* **Entering IDLE at start-up enters OPERATIONAL first**, and HEATING to HOLD neither leaves nor
  re-enters OPERATIONAL, because both are its children. FAULT is not, so the fault event prints
  `OPERATIONAL exit`, and the reset prints `OPERATIONAL entry`.
* **Events a state does not handle go to its parent.** `STOP` in IDLE returns
  `SMF_EVENT_PROPAGATE`, and OPERATIONAL ignores it. `START` in FAULT is ignored by FAULT itself,
  which has no parent.
* **A `k_event` is a set of bits, not a queue.** `k_event_wait_safe()` returns the pending events
  and clears them in one step, and the loop then handles them one bit at a time, lowest bit first.
  Events posted while the heater thread is busy merge: post `START`, `STOP`, `START`, `START` with the
  scheduler locked, and the thread handles `START`, then `START` again, then `STOP`. The first
  `START` went straight to the waiting thread. The other three became two bits, which lost both
  their count and their order. When either matters, use a message queue instead.

---
