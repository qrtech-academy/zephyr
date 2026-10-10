/*
 * SPDX-License-Identifier: MIT
 *
 * Waiting for an interrupt: by blocking, and by spinning, which stalls native_sim.
 */

#include <stdbool.h>

#include <zephyr/kernel.h>

/* Set by the timer's expiry function, in interrupt context. */
static volatile bool expired;
static K_SEM_DEFINE(expired_sem, 0, 1);

static void timer_expired(struct k_timer *timer)
{
	expired = true;
	k_sem_give(&expired_sem);
}

static K_TIMER_DEFINE(timer, timer_expired, NULL);

int main(void)
{
	k_timer_start(&timer, K_MSEC(500), K_NO_WAIT);
	printk("%lld ms: timer started, waiting for it to expire\n", k_uptime_get());

	if (IS_ENABLED(CONFIG_APP_BUSY_LOOP)) {
		/*
		 * On a microcontroller, the timer interrupt preempts this loop after 500 ms of
		 * spinning, sets the flag, and the loop ends. On native_sim, the loop runs in zero
		 * simulated time: time only moves on when the CPU goes idle, and a thread spinning
		 * here never lets it. The timer never expires, and nothing else ever runs again.
		 */
		printk("spinning until the flag is set\n");
		while (!expired) {
		}
	} else {
		/*
		 * Blocking lets the CPU go idle, time passes, the timer interrupt arrives, and the
		 * semaphore wakes this thread. It is also what a microcontroller should do, because
		 * a spinning CPU burns power for nothing.
		 */
		printk("blocking on a semaphore\n");
		k_sem_take(&expired_sem, K_FOREVER);
	}

	printk("%lld ms: timer expired\n", k_uptime_get());
	return 0;
}
