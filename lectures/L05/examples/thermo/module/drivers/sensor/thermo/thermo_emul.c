/*
 * SPDX-License-Identifier: MIT
 *
 * Emulator for the Thermo I2C temperature sensor: the chip's register map, in RAM.
 */

/* The same compatible as the driver: one emulator for every node the driver has a device for. */
#define DT_DRV_COMPAT qacademy_thermo

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

#include <qacademy/thermo_emul.h>

#include "thermo_reg.h"

LOG_MODULE_DECLARE(thermo, CONFIG_SENSOR_LOG_LEVEL);

#define THERMO_NUM_REGS 16

struct thermo_emul_data {
	/* The registers, as the driver sees them over I2C. */
	uint8_t reg[THERMO_NUM_REGS];
	/* The temperature around the emulated part, which the next conversion measures. */
	int16_t ambient_raw;
	/* Fault injection: conversions never finish. */
	bool stuck;
};

/* A conversion: measure the ambient temperature into TEMP, and clear ONE_SHOT unless stuck. */
static void thermo_emul_convert(struct thermo_emul_data *data)
{
	uint16_t value = (uint16_t)data->ambient_raw << THERMO_TEMP_SHIFT;

	data->reg[THERMO_REG_TEMP_MSB] = value >> 8;
	data->reg[THERMO_REG_TEMP_LSB] = value & 0xFF;
	if (!data->stuck) {
		data->reg[THERMO_REG_CTRL] &= ~THERMO_CTRL_ONE_SHOT;
	}
}

static void thermo_emul_write(struct thermo_emul_data *data, uint8_t reg, uint8_t value)
{
	/* Only CTRL is writable; writes to the other registers are ignored, as on the part. */
	if (reg != THERMO_REG_CTRL) {
		LOG_WRN("write of 0x%02x to read-only register 0x%02x ignored", value, reg);
		return;
	}

	data->reg[THERMO_REG_CTRL] = value;
	if ((value & THERMO_CTRL_ONE_SHOT) != 0) {
		/* The emulated part converts instantly; a real one takes about 30 ms. */
		thermo_emul_convert(data);
	}
}

/*
 * Called by the emulated I2C controller for every transfer addressed to this part. The driver's
 * i2c_reg_write_byte_dt() arrives as one write message, [register, value]. Its
 * i2c_reg_read_byte_dt() and i2c_burst_read_dt() arrive as two messages: a write of the register
 * number, then a read of one or more bytes, continuing at the following registers.
 */
static int thermo_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				int addr)
{
	struct thermo_emul_data *data = target->data;
	uint8_t reg;

	ARG_UNUSED(addr);

	if ((num_msgs < 1) || (msgs[0].len < 1) || ((msgs[0].flags & I2C_MSG_READ) != 0)) {
		LOG_ERR("transfer does not start with a register number");
		return -EIO;
	}
	reg = msgs[0].buf[0];

	if (num_msgs == 1) {
		for (size_t i = 1; i < msgs[0].len; i++) {
			thermo_emul_write(data, reg + i - 1, msgs[0].buf[i]);
		}
		return 0;
	}

	if ((num_msgs == 2) && ((msgs[1].flags & I2C_MSG_READ) != 0)) {
		for (size_t i = 0; i < msgs[1].len; i++) {
			if (reg + i >= THERMO_NUM_REGS) {
				return -EIO;
			}
			msgs[1].buf[i] = data->reg[reg + i];
		}
		return 0;
	}

	LOG_ERR("unsupported transfer of %d messages", num_msgs);
	return -EIO;
}

/*
 * The generic sensor emulator backend: set the temperature the part will measure. The value is a
 * fraction in Q31 format scaled by 2^shift, in degrees Celsius, so that one function signature
 * serves every kind of sensor. The part can only measure in steps of 1/16 degree, so the value is
 * rounded to the nearest step, as a real converter would.
 */
static int thermo_emul_set_channel(const struct emul *target, struct sensor_chan_spec ch,
				   const q31_t *value, int8_t shift)
{
	struct thermo_emul_data *data = target->data;
	int64_t micro_c;
	int64_t raw;

	if ((ch.chan_type != SENSOR_CHAN_AMBIENT_TEMP) || (ch.chan_idx != 0)) {
		return -ENOTSUP;
	}

	/*
	 * value / 2^31 * 2^shift degrees, in millionths. Dividing by 2^(31 - shift) instead of
	 * multiplying by 2^shift first keeps every intermediate within 64 bits.
	 */
	if ((shift < 0) || (shift > 31)) {
		return -EINVAL;
	}
	micro_c = (int64_t)*value * 1000000 / (1LL << (31 - shift));

	if (micro_c >= 0) {
		raw = (micro_c + THERMO_MICRO_C_PER_LSB / 2) / THERMO_MICRO_C_PER_LSB;
	} else {
		raw = (micro_c - THERMO_MICRO_C_PER_LSB / 2) / THERMO_MICRO_C_PER_LSB;
	}

	if ((raw < THERMO_RAW_MIN) || (raw > THERMO_RAW_MAX)) {
		return -ERANGE;
	}

	data->ambient_raw = (int16_t)raw;
	return 0;
}

/* The range and resolution of the part, in the same format, for generic sensor tests. */
static int thermo_emul_get_sample_range(const struct emul *target, struct sensor_chan_spec ch,
					q31_t *lower, q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	ARG_UNUSED(target);

	if ((ch.chan_type != SENSOR_CHAN_AMBIENT_TEMP) || (ch.chan_idx != 0)) {
		return -ENOTSUP;
	}

	/* With a shift of 7, Q31 covers -128 up to (not including) +128 degrees. */
	*shift = 7;
	*lower = INT32_MIN;
	*upper = THERMO_RAW_MAX * (1 << 20);
	*epsilon = 1 << 20; /* 1/16 degree: 2^31 / 2^7 / 2^4. */

	return 0;
}

void thermo_emul_set_who_am_i(const struct emul *target, uint8_t value)
{
	struct thermo_emul_data *data = target->data;

	data->reg[THERMO_REG_WHO_AM_I] = value;
}

void thermo_emul_set_stuck(const struct emul *target, bool stuck)
{
	struct thermo_emul_data *data = target->data;

	data->stuck = stuck;
}

/*
 * Runs when the emulated I2C controller initializes, which is before any driver on the bus: the
 * part is in its reset state by the time thermo_init() reads WHO_AM_I.
 */
static int thermo_emul_init(const struct emul *target, const struct device *parent)
{
	struct thermo_emul_data *data = target->data;

	ARG_UNUSED(parent);

	memset(data->reg, 0, sizeof(data->reg));
	data->reg[THERMO_REG_WHO_AM_I] = THERMO_WHO_AM_I_VALUE;
	data->ambient_raw = 0;
	data->stuck = false;

	return 0;
}

static const struct i2c_emul_api thermo_emul_i2c_api = {
	.transfer = thermo_emul_transfer,
};

static const struct emul_sensor_driver_api thermo_emul_sensor_api = {
	.set_channel = thermo_emul_set_channel,
	.get_sample_range = thermo_emul_get_sample_range,
};

#define THERMO_EMUL_DEFINE(inst)                                                                   \
	static struct thermo_emul_data thermo_emul_data_##inst;                                    \
	EMUL_DT_INST_DEFINE(inst, thermo_emul_init, &thermo_emul_data_##inst, NULL,                \
			    &thermo_emul_i2c_api, &thermo_emul_sensor_api)

DT_INST_FOREACH_STATUS_OKAY(THERMO_EMUL_DEFINE)
