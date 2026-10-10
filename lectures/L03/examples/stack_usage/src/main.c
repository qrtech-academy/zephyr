/*
 * SPDX-License-Identifier: MIT
 *
 * A thread whose stack is too small, which native_sim cannot notice and -fstack-usage can.
 */

#include <stdio.h>

#include <zephyr/kernel.h>

/* The stack size given to the reporter thread. */
#define REPORTER_STACK_SIZE 1024

/*
 * Formats a report into a local buffer. The buffer alone is twice the size of the stack of the
 * thread that calls this function.
 */
static void print_report(int round)
{
	char report[2048];

	snprintf(report, sizeof(report), "round %d: report of %zu bytes, on a stack of %d bytes",
		 round, sizeof(report), REPORTER_STACK_SIZE);
	printk("%s\n", report);
}

static void reporter_entry(void *p1, void *p2, void *p3)
{
	for (int round = 1; round <= 3; round++) {
		print_report(round);
		k_msleep(100);
	}
}

/*
 * On a microcontroller, print_report() would write far past the end of this thread's stack, into
 * whatever lies below it. On native_sim the program runs fine: the thread really runs on a host
 * thread's stack, megabytes deep, and the 1024 bytes reserved here are hardly used.
 */
K_THREAD_DEFINE(reporter, REPORTER_STACK_SIZE, reporter_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	return 0;
}
