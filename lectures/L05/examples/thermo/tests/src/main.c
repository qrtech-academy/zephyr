/*
 * SPDX-License-Identifier: MIT
 *
 * Tests of the Thermo driver, run against its emulator on native_sim's I2C bus.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <qacademy/thermo_emul.h>

#define THERMO_NODE        DT_NODELABEL(thermo)
#define THERMO_BROKEN_NODE DT_NODELABEL(thermo_broken)

static const struct device *const sensor = DEVICE_DT_GET(THERMO_NODE);
static const struct emul *const emul = EMUL_DT_GET(THERMO_NODE);

/* Set the temperature the emulated part will measure, in millionths of a degree Celsius. */
static int set_temperature(int64_t micro_c)
{
	/* The backend takes a Q31 fraction scaled by 2^shift. A shift of 8 spans
	 * +-256 degrees, enough to express temperatures the part cannot measure. */
	const int8_t shift = 8;
	q31_t value = (q31_t)((micro_c * (1LL << (31 - shift))) / 1000000);
	struct sensor_chan_spec chan = {SENSOR_CHAN_AMBIENT_TEMP, 0};

	return emul_sensor_backend_set_channel(emul, chan, &value, shift);
}

/* Fetch a reading and convert it, asserting that neither step fails. */
static struct sensor_value read_temperature(void)
{
	struct sensor_value value;

	zassert_ok(sensor_sample_fetch(sensor));
	zassert_ok(sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &value));

	return value;
}

/* Undo the fault injection of any test, so that every test starts from a working part. */
static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	thermo_emul_set_stuck(emul, false);
}

ZTEST_SUITE(thermo, NULL, NULL, NULL, after, NULL);

ZTEST(thermo, test_device_is_ready)
{
	zassert_true(device_is_ready(sensor), "init failed, although WHO_AM_I is right");
}

ZTEST(thermo, test_positive_temperature)
{
	zassert_ok(set_temperature(21500000));

	struct sensor_value value = read_temperature();

	zassert_equal(value.val1, 21);
	zassert_equal(value.val2, 500000);
}

ZTEST(thermo, test_zero)
{
	zassert_ok(set_temperature(0));

	struct sensor_value value = read_temperature();

	zassert_equal(value.val1, 0);
	zassert_equal(value.val2, 0);
}

ZTEST(thermo, test_negative_temperature)
{
	zassert_ok(set_temperature(-18062500));

	struct sensor_value value = read_temperature();

	/* Both parts of a negative sensor_value are negative. */
	zassert_equal(value.val1, -18);
	zassert_equal(value.val2, -62500);
}

ZTEST(thermo, test_negative_below_one_degree)
{
	zassert_ok(set_temperature(-500000));

	struct sensor_value value = read_temperature();

	/* val1 cannot carry the sign of -0.5, so val2 must. */
	zassert_equal(value.val1, 0);
	zassert_equal(value.val2, -500000);
}

ZTEST(thermo, test_readings_are_in_steps_of_a_sixteenth)
{
	/* 23.1 degrees is 369.6 sixteenths; the part rounds that to 370, which is 23.125. */
	zassert_ok(set_temperature(23100000));

	struct sensor_value value = read_temperature();

	zassert_equal(value.val1, 23);
	zassert_equal(value.val2, 125000);
}

ZTEST(thermo, test_range_limits)
{
	struct sensor_value value;

	zassert_ok(set_temperature(-128000000));
	value = read_temperature();
	zassert_equal(sensor_value_to_micro(&value), -128000000);

	zassert_ok(set_temperature(127937500));
	value = read_temperature();
	zassert_equal(sensor_value_to_micro(&value), 127937500);

	zassert_equal(set_temperature(150000000), -ERANGE, "the part cannot measure 150 degrees");
}

ZTEST(thermo, test_channel_get_returns_the_last_fetch)
{
	struct sensor_value value;

	zassert_ok(set_temperature(10000000));
	zassert_ok(sensor_sample_fetch(sensor));

	/* The world changes, but nobody fetches: channel_get still converts the old reading. */
	zassert_ok(set_temperature(30000000));
	zassert_ok(sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &value));
	zassert_equal(value.val1, 10);

	zassert_ok(sensor_sample_fetch(sensor));
	zassert_ok(sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &value));
	zassert_equal(value.val1, 30);
}

ZTEST(thermo, test_unsupported_channel)
{
	struct sensor_value value;

	zassert_equal(sensor_sample_fetch_chan(sensor, SENSOR_CHAN_HUMIDITY), -ENOTSUP);
	zassert_equal(sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &value), -ENOTSUP);
}

ZTEST(thermo, test_conversion_timeout)
{
	thermo_emul_set_stuck(emul, true);

	int64_t start = k_uptime_get();

	zassert_equal(sensor_sample_fetch(sensor), -ETIMEDOUT);

	/* conversion-timeout-ms = <50> in app.overlay, in simulated time. */
	int64_t elapsed = k_uptime_get() - start;

	zassert_true(elapsed >= 50, "gave up after %lld ms, before the timeout", elapsed);
	zassert_true(elapsed < 100, "waited %lld ms, far beyond the timeout", elapsed);
}

ZTEST(thermo, test_wrong_who_am_i_fails_init)
{
	const struct device *const broken = DEVICE_DT_GET(THERMO_BROKEN_NODE);
	const struct emul *const broken_emul = EMUL_DT_GET(THERMO_BROKEN_NODE);

	/* Deferred: not initialized at boot, so not ready. */
	zassert_false(device_is_ready(broken));

	thermo_emul_set_who_am_i(broken_emul, 0x00);
	zassert_equal(device_init(broken), -ENODEV);
	zassert_false(device_is_ready(broken));

	/* A failed init still counts as run: the device cannot be initialized a second time. */
	thermo_emul_set_who_am_i(broken_emul, 0xA5);
	zassert_equal(device_init(broken), -EALREADY);
	zassert_false(device_is_ready(broken));
}
