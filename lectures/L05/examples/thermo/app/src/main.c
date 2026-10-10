/*
 * SPDX-License-Identifier: MIT
 *
 * Read the Thermo sensor once a second through Zephyr's generic sensor API.
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(thermo_app, LOG_LEVEL_INF);

int main(void)
{
	/* The device the driver created for the node labelled "thermo" in app.overlay. */
	const struct device *const sensor = DEVICE_DT_GET(DT_NODELABEL(thermo));
	struct sensor_value value;
	int ret;

	if (!device_is_ready(sensor)) {
		LOG_ERR("%s is not ready", sensor->name);
		return 0;
	}

	LOG_INF("Reading %s", sensor->name);

	while (true) {
		/* Fetch talks to the part; get only converts what fetch stored. */
		ret = sensor_sample_fetch(sensor);
		if (ret == 0) {
			ret = sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &value);
		}

		if (ret < 0) {
			LOG_ERR("Reading failed: %d", ret);
		} else {
			/*
			 * Both parts of a sensor_value carry the sign, so -0.5 is val1 = 0 and
			 * val2 = -500000: printing them as "%d.%06d" would lose the minus.
			 * Converting to one integer first avoids that. Micro rather than milli,
			 * because the part measures in steps of 1/16 = 0.0625 degrees, which needs
			 * four decimals.
			 */
			int64_t micro = sensor_value_to_micro(&value);

			LOG_INF("Temperature: %s%lld.%04lld C", micro < 0 ? "-" : "",
				llabs(micro) / 1000000, llabs(micro) % 1000000 / 100);
		}

		k_sleep(K_SECONDS(1));
	}

	return 0;
}
