# Appendix B
## Application Architecture: zbus and the State Machine Framework
[Appendix A](./a_kernel_objects.md) gave threads the means to cooperate. This appendix is about
keeping that cooperation manageable as an application grows: **zbus**, which lets the parts of an
application exchange messages without knowing about each other, and the **State Machine Framework**,
which turns *what should happen now?* into a table rather than a tangle of flags. By the end you
should be able to explain every line the [`zbus`](../examples/zbus) and [`smf`](../examples/smf)
examples print.

---

## Why decouple
A temperature sensor thread that calls `display_update()`, `alarm_check()` and `logger_write()`
knows about three other modules, links against them, and has to change whenever a fourth one wants
the temperature. Give it a message queue per consumer instead and it still knows about all of them,
only through their queues. Either way, testing the sensor means building everything it talks to.

The alternative is to make the *data* the interface. The sensor publishes temperatures to a
**channel**; whoever needs temperatures observes that channel. The sensor does not know who is
listening, and the consumers do not know who publishes. Adding a consumer changes no existing code,
and a test can publish fake temperatures to exercise the consumers, or observe the channel to check
the sensor. zbus is Zephyr's implementation of that idea.

---

## zbus
zbus needs `CONFIG_ZBUS=y`. It has two kinds of thing: **channels**, which hold messages, and
**observers**, which are told when a channel changes.

### Channels
A channel is a statically allocated message of one type, a lock, and a list of observers:

```c
struct temperature_msg {
	uint32_t seq;
	int32_t millidegrees;
};

ZBUS_CHAN_DEFINE(temperature_chan,              /* The channel's name. */
		 struct temperature_msg,        /* The message type. */
		 temperature_is_valid,          /* A validator, or NULL. */
		 NULL,                          /* User data, or NULL. */
		 ZBUS_OBSERVERS(alarm, display), /* Who is notified, in this order. */
		 ZBUS_MSG_INIT(0));             /* The message's initial value. */
```

The channel **owns** its message: there is exactly one copy of it, inside the channel, and it always
holds the last value published. Everything else gets copies, or a look while the channel is locked.

A **validator** is a function that checks a message before it is accepted:

```c
static bool temperature_is_valid(const void *msg, size_t msg_size)
{
	const struct temperature_msg *t = msg;

	return (t->millidegrees >= -40000) && (t->millidegrees <= 125000);
}
```

A message the validator rejects is never stored, no observer hears of it, and the publisher gets
`-ENOMSG`. That keeps the check in one place, next to the data, rather than in every consumer.

### Publishing and reading

```c
struct temperature_msg t = {.seq = 1, .millidegrees = 21500};
int err = zbus_chan_pub(&temperature_chan, &t, K_MSEC(100));
```

`zbus_chan_pub()` does five things, in order:
1. Runs the validator, and returns `-ENOMSG` if it rejects the message.
2. Locks the channel, waiting at most the timeout, and returns `-EBUSY` or `-EAGAIN` if it cannot.
3. Copies the message into the channel.
4. Notifies every observer, in the order `ZBUS_OBSERVERS()` lists them.
5. Unlocks the channel.

If notifying an observer fails, for example because a subscriber's queue is full, `zbus_chan_pub()`
returns that error, **although the message was stored**, and the remaining observers are still
notified. The one exception is running out of memory for a message subscriber's copy, which stops
the notifications there.

Reading copies the message out, under the lock:

```c
struct temperature_msg t;
zbus_chan_read(&temperature_chan, &t, K_MSEC(100));
```

`zbus_chan_claim()` and `zbus_chan_finish()` lock the channel and give access to the message in
place, for large messages that should not be copied; hold the claim briefly, since it blocks every
publisher and reader.

**A note on error numbers.** The [`zbus`](../examples/zbus) example compares the return value with
`-ENOMSG` rather than printing it, because the number depends on the C library. `native_sim` uses
the host's, where `ENOMSG` is 42; Zephyr's own minimal C library defines it as 35. Always compare
with the name.

### Observers
There are four kinds of observer. What distinguishes them is **which thread runs their code**, and
**what they receive**:

| Observer | Defined with | Runs in | Receives |
|----------|--------------|---------|----------|
| Listener | `ZBUS_LISTENER_DEFINE(name, callback)` | The publisher's thread, inside `zbus_chan_pub()`, with the channel locked | A call; it reads the message in place with `zbus_chan_const_msg()` |
| Subscriber | `ZBUS_SUBSCRIBER_DEFINE(name, queue_size)` | A thread of its own, waiting in `zbus_sub_wait()` | A notification that the channel changed; it reads the channel itself |
| Message subscriber | `ZBUS_MSG_SUBSCRIBER_DEFINE(name)` | A thread of its own, waiting in `zbus_sub_wait_msg()` | A copy of every message |
| Asynchronous listener | `ZBUS_ASYNC_LISTENER_DEFINE(name, callback)` | A work queue | A copy of the message, passed to the callback |

**A listener is synchronous.** Its callback runs before `zbus_chan_pub()` returns, in the publisher's
thread, with the channel locked, so it is effectively part of the publisher. In the example,
`alarm saw #1 ... in thread 'sensor_thread'` is printed before `zbus_chan_pub() returned 0`. A
listener must therefore be short and must never block, just like an interrupt handler, and it
cannot publish to its own channel, whose lock its thread already holds. A listener is the right
tool for something quick and urgent: setting an alarm output, waking a thread.

**A subscriber is told, not given.** Its notification is a pointer to the channel, put into a
message queue of `queue_size` entries. The subscriber's thread reads the channel when it gets to
run, and sees whatever the channel holds *then*:

```c
ZBUS_SUBSCRIBER_DEFINE(display, 4);

static void display_main(void)
{
	const struct zbus_channel *chan;
	struct temperature_msg t;

	while (zbus_sub_wait(&display, &chan, K_FOREVER) == 0) {
		zbus_chan_read(chan, &t, K_FOREVER);
		show(&t);
	}
}
```

**So a subscriber can miss messages.** If two messages are published before the subscriber's thread
runs, both notifications are waiting, and both reads return the second message. In the example, the
display sees #3 twice and never sees #2. That is not a bug: for a display, only the latest value
matters, and skipping stale ones is exactly right. When a subscriber's queue is full, the publisher
waits for room for as long as its timeout allows, **still holding the channel's lock**, so the
subscriber cannot read the channel to catch up; when the timeout expires, the notification is lost
and `zbus_chan_pub()` returns the error. Size the queue for the bursts the subscriber must survive.

**A message subscriber gets everything.** zbus copies each message into a buffer (a `net_buf`, from
the heap by default) and queues the copy, and `zbus_sub_wait_msg()` returns it:

```c
ZBUS_MSG_SUBSCRIBER_DEFINE(logger);

while (zbus_sub_wait_msg(&logger, &chan, &t, K_FOREVER) == 0) {
	log_reading(&t);
}
```

In the example, the logger sees both #2 and #3. The cost is a buffer per queued message, and it needs
`CONFIG_ZBUS_MSG_SUBSCRIBER=y`. Use it when every message matters, as for a logger or a protocol.

**Asynchronous listeners** need `CONFIG_ZBUS_ASYNC_LISTENER=y`: their callback runs later, from a
work queue, with a copy of the message. They suit work too slow for a listener that does not
justify a thread of its own.

### Priorities decide the order
A listener's output always appears inside the publish. Everything else happens when the scheduler
says so. In the example, the sensor thread has the highest priority, so the subscribers only run
once it sleeps; give the sensor a lower priority than the subscribers, and each subscriber runs as
soon as its notification is queued, before `zbus_chan_pub()` has even returned, and then the
display sees #2 too.

`CONFIG_ZBUS_PRIORITY_BOOST`, enabled by default, can raise a publisher's priority while it holds
a channel, above the highest-priority observer thread, so that observers cannot preempt a publish
halfway. It only knows an observer's priority once the observer's thread has called
`zbus_obs_attach_to_thread()`; the examples do not, so no boost happens there.

### Designing with channels
* **One channel per kind of data**, named for the data (`temperature_chan`), not for its producer
  or consumer.
* **Small, flat messages.** Every publish and read copies them.
* **Choose the observer by what it needs:** a listener for urgent and quick, a subscriber for *the
  latest value*, a message subscriber for *every value*.
* **Validate in the channel**, not in the consumers.

---

## The State Machine Framework
Most devices are state machines, whether or not their code says so. A heater is idle, heating,
holding a temperature, or in a fault, and what an event means depends on which: *temperature
reached* matters while heating, and means nothing while idle. Written as flags and `if` statements,
that knowledge is spread over every function that handles an event. Written as a state machine, it
is in one place, and each state's code only handles what can happen in that state.

The State Machine Framework (SMF) needs `CONFIG_SMF=y`, and `CONFIG_SMF_ANCESTOR_SUPPORT=y` for
parent states.

### States
A state is a `struct smf_state` with up to three **actions**, functions that take the state machine's
object:
* **entry**, run when the state is entered;
* **run**, run each time the state machine runs while in the state, typically once per event;
* **exit**, run when the state is left.

The whole state machine is an array of states:

```c
enum heater_state { OPERATIONAL, IDLE, HEATING, HOLD, FAULT };

static const struct smf_state states[] = {
	[OPERATIONAL] = SMF_CREATE_STATE(operational_entry, operational_run,
					 operational_exit, NULL, NULL),
	[IDLE] = SMF_CREATE_STATE(idle_entry, idle_run, NULL,
				  &states[OPERATIONAL], NULL),
	[HEATING] = SMF_CREATE_STATE(heating_entry, heating_run, heating_exit,
				     &states[OPERATIONAL], NULL),
	[HOLD] = SMF_CREATE_STATE(hold_entry, hold_run, NULL,
				  &states[OPERATIONAL], NULL),
	[FAULT] = SMF_CREATE_STATE(fault_entry, fault_run, fault_exit, NULL, NULL),
};
```

`SMF_CREATE_STATE(entry, run, exit, parent, initial)` takes the three actions, any of which may be
`NULL`, the state's parent, and an initial child state, used only with
`CONFIG_SMF_INITIAL_TRANSITION`.

### The object and its context
The state machine works on an object of the application's own, whose **first member** must be a
`struct smf_ctx`, so that a pointer to the object is also a pointer to the context:

```c
struct heater {
	struct smf_ctx ctx;   /* Must be first. */
	uint32_t event;       /* What the run actions look at. */
};
```

`SMF_CTX(&heater)` casts the object to its context for SMF's functions, and every action receives
the object as a `void *`.

### Running it
Three functions do everything:
* **`smf_set_initial(ctx, state)`** starts the state machine in a state, running the entry actions
  of the state and its ancestors, outermost first.
* **`smf_run_state(ctx)`** runs the current state's run action once.
* **`smf_set_state(ctx, state)`**, called from an action, transitions to another state: it runs the
  exit actions of the states being left and the entry actions of the states being entered.

A typical application runs the state machine once per event, from a thread:

```c
while (true) {
	k_msgq_get(&heater_events, &heater.event, K_FOREVER);
	smf_run_state(SMF_CTX(&heater));
}
```

A run action decides what the event means in its state:

```c
static enum smf_state_result heating_run(void *obj)
{
	struct heater *heater = obj;

	switch (heater->event) {
	case EVENT_TEMP_REACHED:
		smf_set_state(SMF_CTX(heater), &states[HOLD]);
		return SMF_EVENT_HANDLED;
	case EVENT_STOP:
		smf_set_state(SMF_CTX(heater), &states[IDLE]);
		return SMF_EVENT_HANDLED;
	default:
		return SMF_EVENT_PROPAGATE;
	}
}
```

`smf_set_state()` must not be called from an exit action, where the state machine is already in the
middle of a transition; SMF logs an error and ignores it. `smf_set_terminate(ctx, value)` stops the
state machine, and makes `smf_run_state()` return the value from then on.

### Parent states
With `CONFIG_SMF_ANCESTOR_SUPPORT=y`, states form a hierarchy, and two things change.

**Events propagate upwards.** A run action returns `SMF_EVENT_HANDLED` or `SMF_EVENT_PROPAGATE`.
If the state handled the event, or transitioned, that is the end of it. Otherwise the parent's run
action gets the event, then the grandparent's, until one handles it. A state without a run action
propagates everything. Behaviour that several states share is written once, in their parent: in the
example, a fault in IDLE, HEATING or HOLD is handled by OPERATIONAL alone.

**Transitions only leave and enter what changes.** A transition runs exit actions from the current
state up to, but not including, the lowest ancestor that the old and new state have in common, and
entry actions from below that ancestor down to the new state. Moving from HEATING to HOLD, both
children of OPERATIONAL, runs HEATING's exit and HOLD's entry, and OPERATIONAL is neither left nor
entered. Moving to FAULT, which is outside OPERATIONAL, runs OPERATIONAL's exit as well, and the
example's trace shows `OPERATIONAL exit`. Whatever a parent sets up in its entry action, such as
switching a motor on, therefore stays set up while the state machine moves between its children.

Without `CONFIG_SMF_ANCESTOR_SUPPORT`, the parent argument is ignored, every state is on its own,
and a run action's return value does not matter: that is a **flat** state machine.

### Driving it with events
The state machine's thread needs events to run on, and how they arrive matters:
* **A message queue** delivers every event, in order, and can carry data with it. It is the default
  choice.
* **A `k_event`** delivers *which* events happened, as bits. It is simple and ISR-friendly, but
  events of the same kind merge and their order is lost: the [`smf`](../examples/smf) example's
  README shows `START`, `STOP`, `START`, `START` being handled as `START`, `START`, `STOP`. Use it
  only when that cannot matter.
* **zbus**: the state machine's thread subscribes to the channels whose messages are its events.

**Keep the state machine itself free of threads.** If its code only reacts to the event it is given
and records what it does, it can be tested by calling it directly with a sequence of events, with no
threads and no timing at all. Exercise Set 4 is built that way.

---

## Summary
* zbus decouples an application's parts: publishers and observers only know channels.
* A channel owns one statically allocated message of one type, and a validator can reject messages
  before they are stored.
* `zbus_chan_pub()` validates, locks, copies, notifies observers in order, and unlocks.
* A listener runs inside the publish, in the publisher's thread, with the channel locked: short, and
  never blocking.
* A subscriber is notified and reads the latest value, so it can miss intermediate messages. A
  message subscriber gets a copy of every message.
* Thread priorities decide when subscribers run, relative to the publisher.
* Compare error codes with their names: `native_sim` uses the host's C library.
* SMF describes a state machine as a table of states with entry, run and exit actions.
* `smf_set_initial()` starts it, `smf_run_state()` runs it once per event, and `smf_set_state()`
  transitions.
* With parent states, unhandled events propagate upwards, and transitions do not leave or re-enter
  common ancestors.
* Feed a state machine from a message queue when order and count matter, and keep its logic free of
  threads, so that it can be tested directly.

---
