/*
 * SPDX-License-Identifier: MIT
 *
 * A boot counter that survives a restart, kept by the settings subsystem on the simulated flash.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* The value in RAM. settings_load() fills it in from flash; settings_save_one() writes it back. */
static uint32_t boot_count;

/*
 * Called by settings_load() once for every stored setting whose name starts with "boot/", with
 * name set to the rest of it. The value is not passed in: the handler reads it with read_cb, into
 * storage of its own, because only the handler knows how big it should be.
 */
static int boot_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;

	if (settings_name_steq(name, "count", &next) && (next == NULL)) {
		if (len != sizeof(boot_count)) {
			return -EINVAL;
		}
		ssize_t read = read_cb(cb_arg, &boot_count, sizeof(boot_count));

		return (read < 0) ? (int)read : 0;
	}
	return -ENOENT;
}

/* Register the handler for the "boot" subtree at compile time, in a table the subsystem walks. */
SETTINGS_STATIC_HANDLER_DEFINE(boot, "boot", NULL, boot_set, NULL, NULL);

int main(void)
{
	int rc = settings_subsys_init();

	if (rc != 0) {
		LOG_ERR("settings_subsys_init() failed: %d", rc);
		return 0;
	}

	/* On the very first boot there is nothing stored, boot_set() is never called, and 0 stays.
	 */
	rc = settings_load();
	if (rc != 0) {
		LOG_ERR("settings_load() failed: %d", rc);
		return 0;
	}

	boot_count++;

	rc = settings_save_one("boot/count", &boot_count, sizeof(boot_count));
	if (rc != 0) {
		LOG_ERR("settings_save_one() failed: %d", rc);
		return 0;
	}

	LOG_INF("Boot count: %u", boot_count);
	return 0;
}
