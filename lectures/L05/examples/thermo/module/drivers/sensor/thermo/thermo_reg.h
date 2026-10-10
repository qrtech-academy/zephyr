/*
 * SPDX-License-Identifier: MIT
 *
 * The register map of the Thermo temperature sensor, shared by the driver and the emulator.
 */

#ifndef THERMO_REG_H_
#define THERMO_REG_H_

#include <zephyr/sys/util.h>

/*
 * Register  Name      Access  Contents
 * 0x00      TEMP_MSB  R       Temperature, high byte.
 * 0x01      TEMP_LSB  R       Temperature, low byte. The two bytes hold a 12-bit two's complement
 *                             value, left-justified (the low 4 bits read as 0), in steps of
 *                             1/16 degree Celsius: -128.0 to +127.9375.
 * 0x02      CTRL      R/W     Bit 0, ONE_SHOT: write 1 to start a conversion. It reads as 1 until
 *                             the conversion is done and TEMP holds the new value.
 * 0x0F      WHO_AM_I  R       Always 0xA5 on a genuine part.
 *
 * Reads of several bytes continue at the next register, so TEMP_MSB and TEMP_LSB are read in one
 * transfer, and a conversion can never land between the two bytes.
 */
#define THERMO_REG_TEMP_MSB 0x00
#define THERMO_REG_TEMP_LSB 0x01
#define THERMO_REG_CTRL     0x02
#define THERMO_REG_WHO_AM_I 0x0F

#define THERMO_CTRL_ONE_SHOT BIT(0)

#define THERMO_WHO_AM_I_VALUE 0xA5

/* How far the 12-bit value is shifted left in TEMP_MSB:TEMP_LSB. */
#define THERMO_TEMP_SHIFT 4

/* One step of the 12-bit value, in millionths of a degree Celsius: 1/16 degree. */
#define THERMO_MICRO_C_PER_LSB 62500

#define THERMO_RAW_MIN (-2048)
#define THERMO_RAW_MAX 2047

#endif /* THERMO_REG_H_ */
