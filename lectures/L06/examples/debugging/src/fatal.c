/*
 * SPDX-License-Identifier: MIT
 *
 * The application's fatal error handler, which decides what a fatal error costs.
 */

#include <zephyr/fatal.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_REGISTER(fatal, LOG_LEVEL_INF);

/*
 * Zephyr calls this after it has printed what went wrong, for every fatal error: an oops, a panic,
 * a failed assertion, a CPU exception on a board. The kernel's own version, a weak function, halts
 * the system. Defining one replaces it. If it returns, the kernel aborts only the thread that
 * caused the error, and the rest of the system keeps running.
 */
void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	/* On a board, logging is deferred to a thread that may never run again: print now. */
	LOG_PANIC();

	LOG_ERR("Caught fatal error %u in thread '%s'", reason, k_thread_name_get(k_current_get()));

	if (reason == K_ERR_KERNEL_OOPS) {
		LOG_ERR("An oops: aborting only that thread");
		return;
	}

	/* A panic means the kernel itself cannot go on. Returning from one is not allowed. */
	LOG_ERR("Halting the system");
	k_fatal_halt(reason);
}
