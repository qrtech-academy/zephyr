/*
 * SPDX-License-Identifier: MIT
 *
 * Priority inversion, produced with a semaphore and fixed with a mutex.
 */

#include <zephyr/kernel.h>

/* Lower numbers are higher priorities. All three threads are preemptible. */
#define HIGH_PRIORITY   5
#define MEDIUM_PRIORITY 6
#define LOW_PRIORITY    7

/* How long each thread computes, in milliseconds of CPU time. */
#define LOW_WORK_MS    100
#define MEDIUM_WORK_MS 500

#ifdef CONFIG_APP_USE_MUTEX
K_MUTEX_DEFINE(lock);
#define LOCK_NAME   "mutex"
#define lock_take() k_mutex_lock(&lock, K_FOREVER)
#define lock_give() k_mutex_unlock(&lock)
#else
/* A binary semaphore, initially available, used as a lock. It has no owner, so no inheritance. */
K_SEM_DEFINE(lock, 1, 1);
#define LOCK_NAME   "semaphore"
#define lock_take() k_sem_take(&lock, K_FOREVER)
#define lock_give() k_sem_give(&lock)
#endif

static void trace(const char *thread, const char *what)
{
	printk("[%4lld ms] %-6s (priority %d) %s\n", k_uptime_get(), thread,
	       k_thread_priority_get(k_current_get()), what);
}

/*
 * Stand in for computation that takes CPU time. On native_sim code runs in zero simulated time, so
 * a loop of instructions would take no time at all. k_busy_wait() advances the clock without
 * giving up the CPU, and it lets interrupts in, so a thread woken meanwhile can preempt this one.
 * One millisecond at a time, so that work interrupted by a preemption is not counted as done.
 */
static void compute(int ms)
{
	for (int i = 0; i < ms; i++) {
		k_busy_wait(USEC_PER_MSEC);
	}
}

static void low_thread(void)
{
	trace("low", "takes the " LOCK_NAME);
	lock_take();
	compute(LOW_WORK_MS);
	trace("low", "gives the " LOCK_NAME " back");
	lock_give();
	trace("low", "is done");
}

static void medium_thread(void)
{
	trace("medium", "starts computing, needs no lock");
	compute(MEDIUM_WORK_MS);
	trace("medium", "is done");
}

static void high_thread(void)
{
	int64_t start = k_uptime_get();

	trace("high", "wants the " LOCK_NAME);
	lock_take();
	trace("high", "has the " LOCK_NAME);
	printk("high waited %lld ms for the %s\n", k_uptime_get() - start, LOCK_NAME);
	lock_give();
}

/*
 * The start delays set up the scenario: low runs first and takes the lock, high arrives 20 ms
 * later and blocks on it, and medium arrives 10 ms after that, while low still holds the lock.
 */
K_THREAD_DEFINE(low, 1024, low_thread, NULL, NULL, NULL, LOW_PRIORITY, 0, 0);
K_THREAD_DEFINE(high, 1024, high_thread, NULL, NULL, NULL, HIGH_PRIORITY, 0, 20);
K_THREAD_DEFINE(medium, 1024, medium_thread, NULL, NULL, NULL, MEDIUM_PRIORITY, 0, 30);
