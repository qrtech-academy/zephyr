/*
 * SPDX-License-Identifier: MIT
 *
 * A sunrise around every emulated Lumo part: the light level changes half a second before each
 * whole second of uptime, so that a program reading once a second sees one new level per reading.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* The light level before each reading, in thousandths of a lux. */
static const int32_t profile_milli_lux[] = {
	0, 300, 42100, 320000, 2750000, 12000000, 30000000,
};

#define LUMO_EMUL_GET(node_id) EMUL_DT_GET(node_id),

static const struct emul *const emulators[] = {
	DT_FOREACH_STATUS_OKAY(qacademy_lumo, LUMO_EMUL_GET)};

static void set_light(int32_t milli_lux)
{
	/* Q31 scaled by 2^15 spans 0 up to 32768 lux. */
	const int8_t shift = 15;
	const q31_t value = (q31_t)(((int64_t)milli_lux * (1LL << (31 - shift))) / 1000);
	const struct sensor_chan_spec chan = {SENSOR_CHAN_LIGHT, 0};

	ARRAY_FOR_EACH(emulators, i) {
		(void)emul_sensor_backend_set_channel(emulators[i], chan, &value, shift);
	}
}

static void daylight(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	set_light(profile_milli_lux[0]);
	for (size_t i = 1; i < ARRAY_SIZE(profile_milli_lux); i++) {
		k_sleep(K_TIMEOUT_ABS_MS(i * 1000 - 500));
		set_light(profile_milli_lux[i]);
	}
}

/* Cooperative, so that it sets the first level before main() takes its first reading. */
K_THREAD_DEFINE(lumo_daylight, 1024, daylight, NULL, NULL, NULL, -1, 0, 0);
