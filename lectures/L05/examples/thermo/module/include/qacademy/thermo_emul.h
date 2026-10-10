/*
 * SPDX-License-Identifier: MIT
 *
 * Controls of the Thermo emulator that no real part has, for tests that need a faulty chip.
 *
 * The temperature itself is not set here: the emulator implements Zephyr's generic sensor
 * emulator backend, so tests set it with emul_sensor_backend_set_channel(), which works the same
 * for every sensor emulator that supports it.
 */

#ifndef QACADEMY_THERMO_EMUL_H_
#define QACADEMY_THERMO_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

/**
 * @brief Set the value the emulated part reports in WHO_AM_I.
 *
 * @param target The emulator.
 * @param value  The value to report. A genuine part reports THERMO_WHO_AM_I_VALUE, 0xA5.
 */
void thermo_emul_set_who_am_i(const struct emul *target, uint8_t value);

/**
 * @brief Make the emulated part's conversions never finish, or finish again.
 *
 * @param target The emulator.
 * @param stuck  If true, CTRL.ONE_SHOT stays set after a conversion is started.
 */
void thermo_emul_set_stuck(const struct emul *target, bool stuck);

#endif /* QACADEMY_THERMO_EMUL_H_ */
