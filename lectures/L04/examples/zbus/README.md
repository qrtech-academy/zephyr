# zbus
A `sensor` thread publishes temperatures on a zbus channel, and three observers receive them: a
listener (`alarm`), a subscriber (`display`) and a message subscriber (`logger`). A validator on the
channel rejects readings outside the sensor's range. The publisher and the observers never refer to
each other, only to the channel.

```bash
west build -p -b native_sim/native/64 lectures/L04/examples/zbus
./build/zephyr/zephyr.exe -stop_at=1 -no-rt
```

---

## What to look at
* **The listener runs inside `zbus_chan_pub()`**, in the publisher's thread: `alarm saw #1 ... in
  thread 'sensor_thread'` comes before `zbus_chan_pub() returned 0`. A listener is a callback, and
  it runs while the channel is locked, so it must be short and must never block.
* **The display sees #3 twice, and never sees #2.** A subscriber is only told that the channel
  changed. When `display_thread` finally runs, both notifications are waiting, and both reads return
  the message that is in the channel by then.
* **The logger sees #2 and #3.** A message subscriber gets its own copy of every message, at the cost
  of a buffer for each one (`CONFIG_ZBUS_MSG_SUBSCRIBER` uses a net_buf pool).
* **Reading #4 (200000 mC) is rejected**: `zbus_chan_pub()` returns `-ENOMSG`, and no observer hears
  about it. The example compares the return value with `-ENOMSG` rather than printing a number,
  because `native_sim` uses the host's C library, where `ENOMSG` is 42. In Zephyr's own minimal C
  library it is 35.
* **The order depends on thread priorities.** Change the sensor's priority from 5 to 8, below both
  subscribers, and each of them runs as soon as it is notified, before `zbus_chan_pub()` has even
  returned. The display then sees #2 as well.

---
