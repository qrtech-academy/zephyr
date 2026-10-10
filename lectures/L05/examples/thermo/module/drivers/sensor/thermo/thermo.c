/*
 * SPDX-License-Identifier: MIT
 *
 * Driver for the Thermo I2C temperature sensor, one device per devicetree node.
 */

/*
 * The compatible this driver serves, with the comma and dash turned into underscores. Every
 * DT_INST_ macro below counts instances of this compatible only.
 */
#define DT_DRV_COMPAT qacademy_thermo

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "thermo_reg.h"

LOG_MODULE_REGISTER(thermo, CONFIG_SENSOR_LOG_LEVEL);

/*
 * What the devicetree says about one instance. It is const, filled in at compile time, and lives
 * in flash on a real board: dev->config.
 */
struct thermo_config {
	struct i2c_dt_spec i2c;
	uint32_t conversion_timeout_ms;
};

/* What changes at run time for one instance: dev->data. */
struct thermo_data {
	/* The last reading, as the 12-bit value in steps of 1/16 degree. */
	int16_t raw;
};

/*
 * Start a conversion, wait for it, and store the result. This is the half of the sensor API that
 * talks to the hardware, and so the half that can fail.
 */
static int thermo_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct thermo_config *config = dev->config;
	struct thermo_data *data = dev->data;
	uint8_t ctrl;
	uint8_t buf[2];
	int64_t deadline;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		return -ENOTSUP;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, THERMO_REG_CTRL, THERMO_CTRL_ONE_SHOT);
	if (ret < 0) {
		return ret;
	}

	/* Poll until the part clears ONE_SHOT, sleeping between reads rather than spinning. */
	deadline = k_uptime_get() + config->conversion_timeout_ms;
	while (true) {
		ret = i2c_reg_read_byte_dt(&config->i2c, THERMO_REG_CTRL, &ctrl);
		if (ret < 0) {
			return ret;
		}
		if ((ctrl & THERMO_CTRL_ONE_SHOT) == 0) {
			break;
		}
		if (k_uptime_get() >= deadline) {
			LOG_ERR("%s: conversion did not finish in %u ms", dev->name,
				config->conversion_timeout_ms);
			return -ETIMEDOUT;
		}
		k_msleep(1);
	}

	/* Both bytes in one transfer, so they belong to the same conversion. */
	ret = i2c_burst_read_dt(&config->i2c, THERMO_REG_TEMP_MSB, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	/*
	 * Undo the left-justification. The cast to int16_t makes the shift arithmetic, which keeps
	 * the sign of a negative temperature (GCC guarantees this for signed types).
	 */
	data->raw = (int16_t)sys_get_be16(buf) >> THERMO_TEMP_SHIFT;

	return 0;
}

/*
 * Convert the stored reading into a struct sensor_value. No I/O, so it cannot fail for a supported
 * channel, and every call returns the same value until the next fetch.
 */
static int thermo_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct thermo_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* -1.5 degrees becomes val1 = -1, val2 = -500000: both parts carry the sign. */
	return sensor_value_from_micro(val, (int64_t)data->raw * THERMO_MICRO_C_PER_LSB);
}

/* The sensor API: the table of functions that sensor_sample_fetch() and friends call through. */
static DEVICE_API(sensor, thermo_api) = {
	.sample_fetch = thermo_sample_fetch,
	.channel_get = thermo_channel_get,
};

/*
 * Runs once at boot, at the level and priority given in THERMO_DEFINE below. A negative return
 * marks the device as failed: device_is_ready() returns false for it from then on.
 */
static int thermo_init(const struct device *dev)
{
	const struct thermo_config *config = dev->config;
	uint8_t id;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("%s: I2C bus %s is not ready", dev->name, config->i2c.bus->name);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, THERMO_REG_WHO_AM_I, &id);
	if (ret < 0) {
		LOG_ERR("%s: no answer at address 0x%02x (%d)", dev->name, config->i2c.addr, ret);
		return ret;
	}

	if (id != THERMO_WHO_AM_I_VALUE) {
		LOG_ERR("%s: WHO_AM_I is 0x%02x, expected 0x%02x", dev->name, id,
			THERMO_WHO_AM_I_VALUE);
		return -ENODEV;
	}

	return 0;
}

/*
 * One instance: its data, its config filled in from its devicetree node, and the device itself.
 * POST_KERNEL runs after the kernel is up, and CONFIG_SENSOR_INIT_PRIORITY (90 by default) is
 * later than the I2C controllers (CONFIG_I2C_INIT_PRIORITY), so the bus is ready first.
 */
#define THERMO_DEFINE(inst)                                                                        \
	static struct thermo_data thermo_data_##inst;                                              \
                                                                                                   \
	static const struct thermo_config thermo_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.conversion_timeout_ms = DT_INST_PROP(inst, conversion_timeout_ms),                \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, thermo_init, NULL, &thermo_data_##inst,                 \
				     &thermo_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &thermo_api);

/* Expand THERMO_DEFINE once for every node with compatible "qacademy,thermo" and status okay. */
DT_INST_FOREACH_STATUS_OKAY(THERMO_DEFINE)
