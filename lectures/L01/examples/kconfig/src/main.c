/*
 * SPDX-License-Identifier: MIT
 *
 * An application configured through its own Kconfig options.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "shout.h"

LOG_MODULE_REGISTER(kconfig, LOG_LEVEL_INF);

int main(void)
{
	const char *greeting = CONFIG_APP_GREETING;
	char loud[32];

	/*
	 * IS_ENABLED() is 1 or 0, so both branches are compiled and type-checked in every
	 * configuration, and the compiler removes the one that cannot run. shout() is only linked
	 * in when CONFIG_APP_SHOUT=y, which is fine: in any other configuration no call to it
	 * remains.
	 */
	if (IS_ENABLED(CONFIG_APP_SHOUT)) {
		shout(greeting, loud, sizeof(loud));
		greeting = loud;
	}

	for (int i = 1; i <= CONFIG_APP_REPEAT; i++) {
		LOG_INF("%s (%d of %d)", greeting, i, CONFIG_APP_REPEAT);
		k_msleep(CONFIG_APP_PERIOD_MS);
	}

	return 0;
}
