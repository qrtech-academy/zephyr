/*
 * SPDX-License-Identifier: MIT
 *
 * The smallest useful Zephyr program: print, log, sleep, and look at the clock.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Register this file as a log module named "hello", which logs at level INF and above. */
LOG_MODULE_REGISTER(hello, LOG_LEVEL_INF);

int main(void)
{
	/* printk() writes straight to the console, like printf() without the formatting library. */
	printk("Hello from %s\n", CONFIG_BOARD_TARGET);

	for (int i = 1; i <= 3; i++) {
		LOG_INF("Tick %d at %lld ms", i, k_uptime_get());
		k_sleep(K_MSEC(500));
	}

	LOG_WRN("A warning, in yellow");
	LOG_DBG("A debug message, filtered out at compile time by LOG_LEVEL_INF");

	/* Returning from main() ends the main thread, not the program: the kernel keeps running. */
	return 0;
}
