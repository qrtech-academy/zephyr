/*
 * SPDX-License-Identifier: MIT
 *
 * Blink the board's LED, found through the devicetree, and read it back from the emulated GPIO.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(blinky, LOG_LEVEL_INF);

/*
 * The node the board calls led0. The alias is what makes this file portable: every board that
 * has an LED names one of them led0, wherever it is wired, and the C code never changes.
 */
#define LED_NODE DT_ALIAS(led0)

/*
 * Everything needed to drive the pin, resolved at compile time from the node's gpios property:
 * the controller device, the pin number, and the flags (here GPIO_ACTIVE_HIGH).
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

int main(void)
{
	/* The device exists, but its init function may have failed. Check before using it. */
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("GPIO controller %s is not ready", led.port->name);
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) != 0) {
		LOG_ERR("Could not configure the LED pin");
		return 0;
	}

	LOG_INF("LED is pin %u on %s", led.pin, led.port->name);

	while (true) {
		gpio_pin_toggle_dt(&led);

		/*
		 * There is no LED to look at, so ask the emulated controller what it is driving.
		 * This call exists only in the emulator's API; on a real board you would look at
		 * the board instead.
		 */
		LOG_INF("LED %s", gpio_emul_output_get(led.port, led.pin) ? "on" : "off");

		k_msleep(500);
	}

	return 0;
}
