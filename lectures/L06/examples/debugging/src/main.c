/*
 * SPDX-License-Identifier: MIT
 *
 * Correct code, and five ways to call it wrongly, for the sanitizers and the kernel to catch.
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define CHECKER_STACK_SIZE 1024
#define CHECKER_PRIORITY   5

/*
 * The functions are __noinline so that the compiler cannot see the wrong values at compile time.
 * It would warn about some of them, which is good in real code, but here the point is what
 * happens at run time.
 */

/* Fill a buffer with a pattern. Correct: it writes exactly size bytes. */
static __noinline void fill(char *buf, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		buf[i] = (char)('a' + i);
	}
}

/* Convert millivolts to nanovolts. Correct, as long as the result fits in an int32_t. */
static __noinline int32_t millivolts_to_nanovolts(int32_t millivolts)
{
	return millivolts * 1000000;
}

/* Check a battery reading. The caller promises a percentage, and the assertion holds it to that. */
static __noinline void report_battery(int percent)
{
	__ASSERT(percent <= 100, "a battery cannot be %d %% full", percent);
	LOG_INF("Battery at %d %%", percent);
}

static void stack_example(void)
{
	char buf[8];

	/* The bug: an off-by-one in the size, one byte past the end of buf. */
	size_t size = IS_ENABLED(CONFIG_APP_BUG_STACK_OVERFLOW) ? sizeof(buf) + 1 : sizeof(buf);

	fill(buf, size);
	LOG_INF("Filled %zu bytes", size);
}

static void heap_example(void)
{
	char *name = malloc(16);

	if (name == NULL) {
		return;
	}
	strcpy(name, "sensor");

	/* The bug: freeing the name before its last use rather than after. */
	if (IS_ENABLED(CONFIG_APP_BUG_USE_AFTER_FREE)) {
		free(name);
	}
	LOG_INF("Name starts with '%c'", name[0]);
	if (!IS_ENABLED(CONFIG_APP_BUG_USE_AFTER_FREE)) {
		free(name);
	}
}

static void arithmetic_example(void)
{
	/* The bug: volts where millivolts were expected, 5000 mV is 5 000 000 000 nV. */
	int32_t millivolts = IS_ENABLED(CONFIG_APP_BUG_SIGNED_OVERFLOW) ? 5000 : 5;

	LOG_INF("%d mV is %d nV", millivolts, millivolts_to_nanovolts(millivolts));
}

static void checker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* The bug: a check that finds the configuration broken, and gives up on the thread. */
	if (IS_ENABLED(CONFIG_APP_BUG_OOPS)) {
		LOG_ERR("The configuration is broken");
		k_oops();
	}
	LOG_INF("The configuration is valid");
}

K_THREAD_STACK_DEFINE(checker_stack, CHECKER_STACK_SIZE);
static struct k_thread checker_thread;

static void thread_example(void)
{
	k_thread_create(&checker_thread, checker_stack, CHECKER_STACK_SIZE, checker, NULL, NULL,
			NULL, CHECKER_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&checker_thread, "checker");

	/* An aborted thread counts as finished, so this returns after an oops too. */
	k_thread_join(&checker_thread, K_FOREVER);
}

static void assert_example(void)
{
	/* The bug: a value in tenths of a percent passed where a percentage was expected. */
	report_battery(IS_ENABLED(CONFIG_APP_BUG_ASSERT) ? 420 : 42);
}

int main(void)
{
	stack_example();
	heap_example();
	arithmetic_example();
	thread_example();
	assert_example();

	LOG_INF("main() is still running, and returns");
	return 0;
}
