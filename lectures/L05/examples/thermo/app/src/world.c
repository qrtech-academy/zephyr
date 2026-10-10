/*
 * SPDX-License-Identifier: MIT
 *
 * The physical world around the emulated sensor: the sensor is carried into a freezer.
 *
 * main.c would run unchanged on a board with a real Thermo part. This file is what the board's
 * surroundings would do instead, and it is only built when the sensor is emulated. It sets the
 * temperature through the generic sensor emulator backend, half a second before each reading.
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* The temperature at each reading, in millionths of a degree Celsius. */
static const int32_t profile_micro_c[] = {
	21500000, 21437500, 12250000, 3500000, -500000, -12000000, -18062500,
};

static void set_temperature(const struct emul *emul, int32_t micro_c)
{
	/* The backend takes a Q31 fraction scaled by 2^shift. A shift of 8 spans
	 * +-256 degrees. */
	const int8_t shift = 8;
	q31_t value = (q31_t)(((int64_t)micro_c * (1LL << (31 - shift))) / 1000000);

	(void)emul_sensor_backend_set_channel(
		emul, (struct sensor_chan_spec){SENSOR_CHAN_AMBIENT_TEMP, 0}, &value, shift);
}

static void world_thread(void *p1, void *p2, void *p3)
{
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(thermo));

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Readings happen at 0 s, 1 s, 2 s, ...; set each temperature before its reading. */
	set_temperature(emul, profile_micro_c[0]);
	for (size_t i = 1; i < ARRAY_SIZE(profile_micro_c); i++) {
		k_sleep(K_TIMEOUT_ABS_MS(i * 1000 - 500));
		set_temperature(emul, profile_micro_c[i]);
	}
}

/* Priority -1 is cooperative: it runs before main() at boot, so the first reading sees 21.5. */
K_THREAD_DEFINE(world, 1024, world_thread, NULL, NULL, NULL, -1, 0, 0);
