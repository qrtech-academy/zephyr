/*
 * SPDX-License-Identifier: MIT
 *
 * One zbus channel, one publisher, and the three kinds of observer that can listen to it.
 */

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

struct temperature_msg {
	uint32_t seq;
	int32_t millidegrees;
};

/*
 * The validator runs inside zbus_chan_pub(), before the message is stored or anyone is told about
 * it. Returning false rejects the message, and the publisher gets -ENOMSG.
 */
static bool temperature_is_valid(const void *msg, size_t msg_size)
{
	const struct temperature_msg *t = msg;

	ARG_UNUSED(msg_size);
	return (t->millidegrees >= -40000) && (t->millidegrees <= 125000);
}

/*
 * The channel owns the message: a statically allocated struct temperature_msg, protected by the
 * channel's lock. The observers are known at compile time, and are notified in the order listed.
 */
ZBUS_CHAN_DEFINE(temperature_chan, struct temperature_msg, temperature_is_valid, NULL,
		 ZBUS_OBSERVERS(alarm, display, logger), ZBUS_MSG_INIT(0));

static const char *current_thread(void)
{
	return k_thread_name_get(k_current_get());
}

/*
 * A listener is a callback, run synchronously inside zbus_chan_pub(), in the publisher's thread and
 * with the channel locked. It must be quick and must not block, like an interrupt handler.
 */
static void alarm_callback(const struct zbus_channel *chan)
{
	const struct temperature_msg *t = zbus_chan_const_msg(chan);

	printk("  %-7s saw #%u = %d mC, in thread '%s'%s\n", "alarm", t->seq, t->millidegrees,
	       current_thread(), t->millidegrees > 30000 ? ": TOO HOT" : "");
}

ZBUS_LISTENER_DEFINE(alarm, alarm_callback);

/*
 * A subscriber is only told that the channel changed, through a queue of channel pointers. It reads
 * the channel when its thread gets to run, so it sees the value current at that moment.
 */
ZBUS_SUBSCRIBER_DEFINE(display, 4);

static void display_main(void)
{
	const struct zbus_channel *chan;
	struct temperature_msg t;

	while (zbus_sub_wait(&display, &chan, K_FOREVER) == 0) {
		zbus_chan_read(chan, &t, K_FOREVER);
		printk("  %-7s saw #%u = %d mC, in thread '%s'\n", zbus_obs_name(&display), t.seq,
		       t.millidegrees, current_thread());
	}
}

/* A message subscriber gets its own copy of every message, so it misses none of them. */
ZBUS_MSG_SUBSCRIBER_DEFINE(logger);

static void logger_main(void)
{
	const struct zbus_channel *chan;
	struct temperature_msg t;

	while (zbus_sub_wait_msg(&logger, &chan, &t, K_FOREVER) == 0) {
		printk("  %-7s saw #%u = %d mC, in thread '%s'\n", zbus_obs_name(&logger), t.seq,
		       t.millidegrees, current_thread());
	}
}

static void publish(struct temperature_msg *t, int32_t millidegrees)
{
	t->seq++;
	t->millidegrees = millidegrees;

	printk("sensor publishes #%u = %d mC\n", t->seq, t->millidegrees);
	int err = zbus_chan_pub(&temperature_chan, t, K_MSEC(100));

	if (err == -ENOMSG) {
		printk("sensor: zbus_chan_pub() returned -ENOMSG, the validator rejected it\n");
	} else {
		printk("sensor: zbus_chan_pub() returned %d\n", err);
	}
}

static void sensor_main(void)
{
	struct temperature_msg t = {0};

	publish(&t, 21500);
	k_msleep(100);

	/* Two readings back to back: the sensor does not sleep in between. */
	publish(&t, 30500);
	publish(&t, 31000);
	k_msleep(100);

	/* Out of range. The validator rejects it, and no observer hears about it. */
	publish(&t, 200000);
}

/*
 * The sensor has the highest priority, so the subscribers only run once it sleeps, and the display
 * never sees #2. Give the sensor priority 8 instead, below both subscribers, and each of them runs
 * as soon as it is notified: before zbus_chan_pub() has even returned.
 */
K_THREAD_DEFINE(sensor_thread, 1024, sensor_main, NULL, NULL, NULL, 5, 0, 10);
K_THREAD_DEFINE(logger_thread, 1024, logger_main, NULL, NULL, NULL, 6, 0, 0);
K_THREAD_DEFINE(display_thread, 1024, display_main, NULL, NULL, NULL, 7, 0, 0);
