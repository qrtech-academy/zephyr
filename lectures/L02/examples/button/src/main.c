/*
 * SPDX-License-Identifier: MIT
 *
 * Toggle the LED from a button interrupt, and press the button from a thread.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* The callback is registered with the GPIO driver, which keeps a list of them per controller. */
static struct gpio_callback button_cb;

/* Written in the callback and could be read anywhere, so atomic. */
static atomic_t presses;

/*
 * Called by the GPIO driver when the interrupt configured below fires. On a microcontroller that
 * is an interrupt service routine, so the callback must not sleep or block. The emulated
 * controller calls it from whichever context changed the pin instead, which it reports below.
 */
static void button_pressed(const struct device *port, struct gpio_callback *cb,
			   gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	/* atomic_inc() returns the value from before the increment. */
	atomic_val_t count = atomic_inc(&presses) + 1;

	gpio_pin_toggle_dt(&led);
	LOG_INF("Press %ld (%s): LED %s", (long)count, k_is_in_isr() ? "ISR" : "thread",
		gpio_emul_output_get(led.port, led.pin) ? "on" : "off");
}

/*
 * The finger. There is no button to press, so this thread drives the emulated pin the way a
 * finger would: low (pressed, since the button is active low) for 100 ms, then high again.
 * gpio_emul_input_set() takes the physical level, not the logical one.
 */
static void finger(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_msleep(900);
		gpio_emul_input_set(button.port, button.pin, 0);
		k_msleep(100);
		gpio_emul_input_set(button.port, button.pin, 1);
	}
}

K_THREAD_DEFINE(finger_thread, 1024, finger, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	if (!gpio_is_ready_dt(&button) || !gpio_is_ready_dt(&led)) {
		LOG_ERR("GPIO controller not ready");
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&button, GPIO_INPUT);

	/*
	 * Interrupt on the edge to the *active* level. Because the devicetree says the button is
	 * active low, the driver turns that into a falling edge on the pin: the C code only ever
	 * talks about pressed and released.
	 */
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb, button_pressed, BIT(button.pin));
	gpio_add_callback_dt(&button, &button_cb);

	LOG_INF("Button is pin %u on %s, LED is pin %u", button.pin, button.port->name, led.pin);

	/* Nothing left for main() to do: the finger thread and the callback carry on without it. */
	return 0;
}
