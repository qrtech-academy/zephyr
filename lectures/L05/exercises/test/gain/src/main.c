/*
 * SPDX-License-Identifier: MIT
 *
 * Tests for Exercise Set 3: the gain property, written to the part at init and used in conversion.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include <qacademy/lumo_emul.h>

#define GAIN_MASK 0x03

static const struct device *const light_x4 = DEVICE_DT_GET(DT_NODELABEL(light_x4));
static const struct device *const light_x8 = DEVICE_DT_GET(DT_NODELABEL(light_x8));
static const struct device *const light_default = DEVICE_DT_GET(DT_NODELABEL(light_default));

static struct sensor_value read_counts(const struct device *dev, uint16_t counts)
{
	struct sensor_value value;

	lumo_emul_set_counts(EMUL_DT_GET(DT_NODELABEL(light_x4)), counts);
	lumo_emul_set_counts(EMUL_DT_GET(DT_NODELABEL(light_x8)), counts);
	lumo_emul_set_counts(EMUL_DT_GET(DT_NODELABEL(light_default)), counts);
	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &value));

	return value;
}

ZTEST_SUITE(gain, NULL, NULL, NULL, NULL, NULL);

ZTEST(gain, test_gain_is_written_at_init)
{
	zassert_equal(lumo_emul_get_config(EMUL_DT_GET(DT_NODELABEL(light_x4))) & GAIN_MASK, 2,
		      "a gain of 4 is GAIN = 2 in CONFIG");
	zassert_equal(lumo_emul_get_config(EMUL_DT_GET(DT_NODELABEL(light_x8))) & GAIN_MASK, 3,
		      "a gain of 8 is GAIN = 3 in CONFIG");
	zassert_equal(lumo_emul_get_config(EMUL_DT_GET(DT_NODELABEL(light_default))) & GAIN_MASK, 0,
		      "the default gain of 1 is GAIN = 0 in CONFIG");
}

ZTEST(gain, test_default_gain_is_one)
{
	struct sensor_value value = read_counts(light_default, 4);

	zassert_equal(value.val1, 1, "4 counts at a gain of 1 is 1 lux, not %d.%06d", value.val1,
		      value.val2);
	zassert_equal(value.val2, 0);
}

ZTEST(gain, test_one_count_at_gain_4)
{
	struct sensor_value value = read_counts(light_x4, 1);

	zassert_equal(value.val1, 0);
	zassert_equal(value.val2, 62500, "1 count at a gain of 4 is 0.0625 lux, not %d.%06d",
		      value.val1, value.val2);
}

ZTEST(gain, test_saturated_at_gain_8)
{
	struct sensor_value value = read_counts(light_x8, 0xFFFF);

	zassert_equal(value.val1, 2047,
		      "0xFFFF counts at a gain of 8 is 2047.96875 lux, not %d.%06d", value.val1,
		      value.val2);
	zassert_equal(value.val2, 968750);
}

ZTEST(gain, test_light_set_in_lux_at_gain_4)
{
	/* At a gain of 4, 42.1 lux is 673.6 counts, which the part rounds to 674: 42.125 lux. */
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(light_x4));
	const int8_t shift = 15;
	const q31_t lux = (q31_t)(42100LL * (1LL << (31 - shift)) / 1000);
	struct sensor_value value;

	zassert_ok(emul_sensor_backend_set_channel(
		emul, (struct sensor_chan_spec){SENSOR_CHAN_LIGHT, 0}, &lux, shift));
	zassert_ok(sensor_sample_fetch(light_x4));
	zassert_ok(sensor_channel_get(light_x4, SENSOR_CHAN_LIGHT, &value));
	zassert_equal(value.val1, 42);
	zassert_equal(value.val2, 125000);
}
