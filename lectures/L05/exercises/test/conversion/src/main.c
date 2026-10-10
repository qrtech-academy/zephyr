/*
 * SPDX-License-Identifier: MIT
 *
 * Tests for Exercise Set 2: the sensor API, and the conversion from counts to lux at a gain of 1.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include <qacademy/lumo_emul.h>

static const struct device *const light = DEVICE_DT_GET(DT_NODELABEL(light));
static const struct emul *const emul = EMUL_DT_GET(DT_NODELABEL(light));

/* Make the part report a count, then fetch and convert a reading. */
static struct sensor_value read_counts(uint16_t counts)
{
	struct sensor_value value;

	lumo_emul_set_counts(emul, counts);
	zassert_ok(sensor_sample_fetch(light));
	zassert_ok(sensor_channel_get(light, SENSOR_CHAN_LIGHT, &value));

	return value;
}

ZTEST_SUITE(conversion, NULL, NULL, NULL, NULL, NULL);

ZTEST(conversion, test_darkness)
{
	struct sensor_value value = read_counts(0);

	zassert_equal(value.val1, 0);
	zassert_equal(value.val2, 0);
}

ZTEST(conversion, test_one_count_is_a_quarter_lux)
{
	struct sensor_value value = read_counts(1);

	zassert_equal(value.val1, 0, "1 count is 0.25 lux, not %d.%06d", value.val1, value.val2);
	zassert_equal(value.val2, 250000, "1 count is 0.25 lux, not %d.%06d", value.val1,
		      value.val2);
}

ZTEST(conversion, test_four_counts_are_one_lux)
{
	struct sensor_value value = read_counts(4);

	zassert_equal(value.val1, 1);
	zassert_equal(value.val2, 0);
}

ZTEST(conversion, test_data_is_little_endian)
{
	/* 0x0102 is 258 counts, 64.5 lux. Read big-endian it would be 513 counts, 128.25 lux. */
	struct sensor_value value = read_counts(0x0102);

	zassert_equal(value.val1, 64, "0x0102 counts is 64.5 lux, not %d.%06d", value.val1,
		      value.val2);
	zassert_equal(value.val2, 500000);
}

ZTEST(conversion, test_saturated)
{
	struct sensor_value value = read_counts(0xFFFF);

	zassert_equal(value.val1, 16383, "0xFFFF counts is 16383.75 lux, not %d.%06d", value.val1,
		      value.val2);
	zassert_equal(value.val2, 750000);
}

ZTEST(conversion, test_light_set_in_lux)
{
	/* 42.1 lux is 168.4 counts, which the part rounds to 168: 42 lux. */
	const int8_t shift = 15;
	const q31_t lux = (q31_t)(42100LL * (1LL << (31 - shift)) / 1000);
	struct sensor_value value;

	zassert_ok(emul_sensor_backend_set_channel(
		emul, (struct sensor_chan_spec){SENSOR_CHAN_LIGHT, 0}, &lux, shift));
	zassert_ok(sensor_sample_fetch(light));
	zassert_ok(sensor_channel_get(light, SENSOR_CHAN_LIGHT, &value));
	zassert_equal(value.val1, 42);
	zassert_equal(value.val2, 0);
}

ZTEST(conversion, test_channel_get_returns_the_last_fetch)
{
	struct sensor_value value;

	lumo_emul_set_counts(emul, 8);
	zassert_ok(sensor_sample_fetch(light));

	lumo_emul_set_counts(emul, 400);
	zassert_ok(sensor_channel_get(light, SENSOR_CHAN_LIGHT, &value));
	zassert_equal(value.val1, 2, "channel_get must convert the last fetch, not read the part");
}

ZTEST(conversion, test_fetch_of_the_light_channel)
{
	struct sensor_value value;

	lumo_emul_set_counts(emul, 40);
	zassert_ok(sensor_sample_fetch_chan(light, SENSOR_CHAN_LIGHT));
	zassert_ok(sensor_channel_get(light, SENSOR_CHAN_LIGHT, &value));
	zassert_equal(value.val1, 10);
}

ZTEST(conversion, test_unsupported_channel)
{
	struct sensor_value value;

	zassert_equal(sensor_sample_fetch_chan(light, SENSOR_CHAN_AMBIENT_TEMP), -ENOTSUP);
	zassert_equal(sensor_channel_get(light, SENSOR_CHAN_AMBIENT_TEMP, &value), -ENOTSUP);
}
