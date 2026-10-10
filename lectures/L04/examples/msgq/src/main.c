/*
 * SPDX-License-Identifier: MIT
 *
 * A fast producer and a slow consumer, connected by a message queue.
 */

#include <stdalign.h>

#include <zephyr/kernel.h>

#define QUEUE_LENGTH      4
#define PRODUCE_PERIOD_MS 100
#define CONSUME_TIME_MS   250
#define SAMPLES_PER_PHASE 12

/*
 * What travels through the queue. k_msgq copies the whole struct in and out, so the producer's
 * local variable can go out of scope as soon as k_msgq_put() returns.
 */
struct sample {
	uint32_t seq;
	int64_t timestamp_ms;
};

/* Room for QUEUE_LENGTH samples, allocated statically; the last argument is the alignment. */
K_MSGQ_DEFINE(samples, sizeof(struct sample), QUEUE_LENGTH, alignof(struct sample));

static void producer_thread(void)
{
	uint32_t seq = 0;
	uint32_t dropped = 0;

	/*
	 * Phase 1: never wait. A full queue means the sample is dropped, and the producer keeps its
	 * rate. This is what an interrupt handler must do, since it cannot block.
	 */
	printk("--- K_NO_WAIT: drop when full\n");
	for (int i = 0; i < SAMPLES_PER_PHASE; i++) {
		struct sample s = {.seq = ++seq, .timestamp_ms = k_uptime_get()};

		if (k_msgq_put(&samples, &s, K_NO_WAIT) == 0) {
			printk("[%4lld ms] produced #%u, queue %u/%d\n", s.timestamp_ms, s.seq,
			       k_msgq_num_used_get(&samples), QUEUE_LENGTH);
		} else {
			dropped++;
			printk("[%4lld ms] queue full, dropped #%u (%u dropped)\n", s.timestamp_ms,
			       s.seq, dropped);
		}
		k_msleep(PRODUCE_PERIOD_MS);
	}

	/* Let the consumer empty the queue before the second phase. */
	while (k_msgq_num_used_get(&samples) > 0) {
		k_msleep(10);
	}
	k_msleep(CONSUME_TIME_MS);

	/*
	 * Phase 2: wait for room. Nothing is dropped, but the producer now runs at the consumer's
	 * rate: the full queue pushes back on it.
	 */
	printk("--- K_FOREVER: block when full\n");
	for (int i = 0; i < SAMPLES_PER_PHASE; i++) {
		struct sample s = {.seq = ++seq, .timestamp_ms = k_uptime_get()};

		k_msgq_put(&samples, &s, K_FOREVER);
		printk("[%4lld ms] produced #%u, blocked %lld ms, queue %u/%d\n", k_uptime_get(),
		       s.seq, k_uptime_get() - s.timestamp_ms, k_msgq_num_used_get(&samples),
		       QUEUE_LENGTH);
		k_msleep(PRODUCE_PERIOD_MS);
	}
}

static void consumer_thread(void)
{
	struct sample s;

	while (true) {
		/* Sleeps, using no CPU, until a sample arrives. */
		k_msgq_get(&samples, &s, K_FOREVER);
		printk("[%4lld ms]                    consumed #%u, %lld ms old\n", k_uptime_get(),
		       s.seq, k_uptime_get() - s.timestamp_ms);
		/* Stands in for slow processing, such as writing the sample to flash. */
		k_msleep(CONSUME_TIME_MS);
	}
}

/* The consumer has the higher priority, so it takes a sample the moment one is put. */
K_THREAD_DEFINE(producer, 1024, producer_thread, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(consumer, 1024, consumer_thread, NULL, NULL, NULL, 6, 0, 0);
