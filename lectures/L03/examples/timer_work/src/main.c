/*
 * SPDX-License-Identifier: MIT
 *
 * A periodic timer whose expiry function defers its work to work queues.
 */

#include <zephyr/kernel.h>

/* Print a line with the uptime and the context it runs in: an ISR, or the name of a thread. */
static void trace(const char *what)
{
	const char *where = k_is_in_isr() ? "(ISR)" : k_thread_name_get(k_current_get());

	printk("%4lld ms  %-8s  %s\n", k_uptime_get(), where, what);
}

/* How many samples to take before the timer is stopped. */
#define SAMPLES 3

/* A dedicated work queue, with a thread of its own, for work that is slow. */
#define STORE_STACK_SIZE 1024
#define STORE_PRIORITY   5

K_THREAD_STACK_DEFINE(store_stack, STORE_STACK_SIZE);
static struct k_work_q store_queue;

static void store_handler(struct k_work *work);
static void sample_handler(struct k_work *work);
static void idle_handler(struct k_work *work);
static void timer_expired(struct k_timer *timer);

static K_WORK_DEFINE(store_work, store_handler);
static K_WORK_DEFINE(sample_work, sample_handler);
static K_WORK_DELAYABLE_DEFINE(idle_work, idle_handler);
static K_TIMER_DEFINE(sample_timer, timer_expired, NULL);

/*
 * The timer's expiry function runs in the system clock's interrupt handler. It must not sleep,
 * block or take long, so all it does is hand the real work to a work queue.
 */
static void timer_expired(struct k_timer *timer)
{
	trace("timer expired: submitting sample_work");
	k_work_submit(&sample_work);
}

/*
 * A work handler runs in the work queue's thread, so it may do what an ISR may not: sleep, block on
 * a mutex, log. k_work_submit() puts sample_work on the system work queue, which every subsystem
 * shares, so a handler there should still be quick.
 */
static void sample_handler(struct k_work *work)
{
	static int samples;

	samples++;
	trace("sampling");

	/*
	 * Push the idle timeout back to 250 ms from now. While samples keep coming it never fires;
	 * it fires 250 ms after the last one.
	 */
	k_work_reschedule(&idle_work, K_MSEC(250));

	/* Storing is slow, so it goes to the dedicated queue instead of holding up this one. */
	k_work_submit_to_queue(&store_queue, &store_work);

	if (samples == SAMPLES) {
		k_timer_stop(&sample_timer);
		trace("last sample: timer stopped");
	}
}

static void store_handler(struct k_work *work)
{
	trace("storing, which sleeps for 30 ms");
	k_msleep(30);
	trace("stored");
}

static void idle_handler(struct k_work *work)
{
	trace("idle timeout: no sample for 250 ms");
}

int main(void)
{
	const struct k_work_queue_config config = {.name = "store_q"};

	k_work_queue_start(&store_queue, store_stack, K_THREAD_STACK_SIZEOF(store_stack),
			   STORE_PRIORITY, &config);

	/* First expiry after 100 ms, then every 100 ms. */
	k_timer_start(&sample_timer, K_MSEC(100), K_MSEC(100));
	trace("timer started");

	return 0;
}
