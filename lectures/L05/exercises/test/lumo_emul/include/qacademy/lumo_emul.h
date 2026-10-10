/*
 * SPDX-License-Identifier: MIT
 *
 * Controls of the Lumo emulator that no real part has, for tests.
 *
 * The light level itself can also be set in lux, through Zephyr's generic sensor emulator backend:
 * emul_sensor_backend_set_channel() with SENSOR_CHAN_LIGHT.
 */

#ifndef QACADEMY_LUMO_EMUL_H_
#define QACADEMY_LUMO_EMUL_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

/**
 * @brief Set the value the emulated part reports in PART_ID.
 *
 * @param target The emulator.
 * @param id     The value to report. A genuine part reports 0x4C.
 */
void lumo_emul_set_part_id(const struct emul *target, uint8_t id);

/**
 * @brief Set the count the emulated part reports in DATA, whatever the gain.
 *
 * @param target The emulator.
 * @param counts The 16-bit count. 0xFFFF is a saturated reading.
 */
void lumo_emul_set_counts(const struct emul *target, uint16_t counts);

/**
 * @brief Get the value of the emulated part's CONFIG register, as the driver last wrote it.
 *
 * @param target The emulator.
 *
 * @return The register's value.
 */
uint8_t lumo_emul_get_config(const struct emul *target);

#endif /* QACADEMY_LUMO_EMUL_H_ */
