/*
 * SPDX-License-Identifier: MIT
 *
 * Emulator for the Lumo I2C ambient light sensor: the chip's register map, in RAM.
 *
 * The register map is written out here on purpose, rather than shared with the driver through a
 * header: the emulator is the datasheet, and a driver whose own register header gets an address
 * wrong must fail against it.
 */

#define DT_DRV_COMPAT qacademy_lumo

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

#include <qacademy/lumo_emul.h>

LOG_MODULE_REGISTER(lumo_emul, LOG_LEVEL_INF);

#define REG_CONFIG   0x00
#define REG_DATA_LSB 0x01
#define REG_DATA_MSB 0x02
#define REG_PART_ID  0x0A
#define NUM_REGS     0x10

#define CONFIG_GAIN_MASK 0x03
#define PART_ID_VALUE    0x4C
#define COUNTS_MAX       0xFFFF

struct lumo_emul_data {
	uint8_t config;
	uint8_t part_id;
	/* The light around the part, in millionths of a lux, or a count set directly. */
	int64_t micro_lux;
	uint16_t counts;
	bool counts_set;
};

/* 1, 2, 4 or 8, from the CONFIG register's GAIN bits. */
static unsigned int gain_of(const struct lumo_emul_data *data)
{
	return 1U << (data->config & CONFIG_GAIN_MASK);
}

/* What the part measures: a quarter of a lux per count at a gain of 1, saturating at 0xFFFF. */
static uint16_t measure(const struct lumo_emul_data *data)
{
	int64_t counts;

	if (data->counts_set) {
		return data->counts;
	}

	counts = (data->micro_lux * 4 * gain_of(data) + 500000) / 1000000;
	return (uint16_t)MIN(counts, COUNTS_MAX);
}

static uint8_t read_register(const struct lumo_emul_data *data, uint8_t reg)
{
	switch (reg) {
	case REG_CONFIG:
		return data->config;
	case REG_DATA_LSB:
		return measure(data) & 0xFF;
	case REG_DATA_MSB:
		return measure(data) >> 8;
	case REG_PART_ID:
		return data->part_id;
	default:
		return 0x00;
	}
}

static int lumo_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
			      int addr)
{
	struct lumo_emul_data *data = target->data;
	uint8_t reg;

	ARG_UNUSED(addr);

	if ((num_msgs < 1) || (msgs[0].len < 1) || ((msgs[0].flags & I2C_MSG_READ) != 0)) {
		LOG_ERR("transfer does not start with a register address");
		return -EIO;
	}
	reg = msgs[0].buf[0];

	/* A write: the register address, then the values of it and the registers after it. */
	if (num_msgs == 1) {
		for (size_t i = 1; i < msgs[0].len; i++) {
			if (reg + i - 1 != REG_CONFIG) {
				LOG_WRN("write to read-only register 0x%02x ignored",
					(unsigned int)(reg + i - 1));
				continue;
			}
			data->config = msgs[0].buf[i] & CONFIG_GAIN_MASK;
		}
		return 0;
	}

	/* A read: the register address, then as many bytes as asked for, from it onwards. */
	if ((num_msgs == 2) && ((msgs[1].flags & I2C_MSG_READ) != 0)) {
		for (size_t i = 0; i < msgs[1].len; i++) {
			if (reg + i >= NUM_REGS) {
				return -EIO;
			}
			msgs[1].buf[i] = read_register(data, reg + i);
		}
		return 0;
	}

	LOG_ERR("unsupported transfer of %d messages", num_msgs);
	return -EIO;
}

/* The generic sensor backend: set the light level in lux, as Q31 scaled by 2^shift. */
static int lumo_emul_set_channel(const struct emul *target, struct sensor_chan_spec ch,
				 const q31_t *value, int8_t shift)
{
	struct lumo_emul_data *data = target->data;

	if ((ch.chan_type != SENSOR_CHAN_LIGHT) || (ch.chan_idx != 0)) {
		return -ENOTSUP;
	}
	if ((shift < 0) || (shift > 31) || (*value < 0)) {
		return -EINVAL;
	}

	data->micro_lux = (int64_t)*value * 1000000 / (1LL << (31 - shift));
	data->counts_set = false;
	return 0;
}

void lumo_emul_set_part_id(const struct emul *target, uint8_t id)
{
	struct lumo_emul_data *data = target->data;

	data->part_id = id;
}

void lumo_emul_set_counts(const struct emul *target, uint16_t counts)
{
	struct lumo_emul_data *data = target->data;

	data->counts = counts;
	data->counts_set = true;
}

uint8_t lumo_emul_get_config(const struct emul *target)
{
	const struct lumo_emul_data *data = target->data;

	return data->config;
}

/* The part's reset state: gain 1, darkness, and a genuine PART_ID. */
static int lumo_emul_init(const struct emul *target, const struct device *parent)
{
	struct lumo_emul_data *data = target->data;

	ARG_UNUSED(parent);

	memset(data, 0, sizeof(*data));
	data->part_id = PART_ID_VALUE;

	return 0;
}

static const struct i2c_emul_api lumo_emul_i2c_api = {
	.transfer = lumo_emul_transfer,
};

static const struct emul_sensor_driver_api lumo_emul_sensor_api = {
	.set_channel = lumo_emul_set_channel,
};

#define LUMO_EMUL_DEFINE(inst)                                                                     \
	static struct lumo_emul_data lumo_emul_data_##inst;                                        \
	EMUL_DT_INST_DEFINE(inst, lumo_emul_init, &lumo_emul_data_##inst, NULL,                    \
			    &lumo_emul_i2c_api, &lumo_emul_sensor_api)

DT_INST_FOREACH_STATUS_OKAY(LUMO_EMUL_DEFINE)
