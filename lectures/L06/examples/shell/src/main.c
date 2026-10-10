/*
 * SPDX-License-Identifier: MIT
 *
 * Shell commands of your own: a command with subcommands, and a command with arguments.
 */

#include <stdlib.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* The board's LED, found through the led0 alias in the native_sim devicetree. */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/*
 * Every command handler has the same signature: the shell instance that ran it, which is where its
 * output goes, and the command line split into words. argv[0] is the command's own name. The
 * return value is 0 on success or a negative errno, which the shell reports.
 */
static int cmd_led_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	gpio_pin_set_dt(&led, 1);
	shell_print(sh, "LED on");
	return 0;
}

static int cmd_led_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	gpio_pin_set_dt(&led, 0);
	shell_print(sh, "LED off");
	return 0;
}

static int cmd_led_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* There is no LED to look at, so ask the emulated GPIO controller what it drives the pin
	 * to. */
	shell_print(sh, "LED is %s", gpio_emul_output_get(led.port, led.pin) ? "on" : "off");
	return 0;
}

static int cmd_greet(const struct shell *sh, size_t argc, char **argv)
{
	int times = 1;

	/*
	 * SHELL_CMD_ARG_REGISTER below says the command takes 2 mandatory words (greet and a name)
	 * and 1 optional one, so the shell has already rejected any other count. What is left to
	 * check is what the words say.
	 */
	if (argc == 3) {
		times = atoi(argv[2]);
		if ((times < 1) || (times > 5)) {
			shell_error(sh, "times must be between 1 and 5, not '%s'", argv[2]);
			return -EINVAL;
		}
	}

	for (int i = 0; i < times; i++) {
		shell_print(sh, "Hello, %s!", argv[1]);
	}
	return 0;
}

/*
 * The subcommands of led. The set is built at compile time into a table in flash, which is also
 * what lets the shell complete them when you press Tab.
 */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_led, SHELL_CMD(on, NULL, "Turn the LED on.", cmd_led_on),
			       SHELL_CMD(off, NULL, "Turn the LED off.", cmd_led_off),
			       SHELL_CMD(status, NULL, "Print whether the LED is on.",
					 cmd_led_status),
			       SHELL_SUBCMD_SET_END);

/* A root command with subcommands and no handler of its own: led alone prints its help. */
SHELL_CMD_REGISTER(led, &sub_led, "Control the LED.", NULL);

/* A root command with arguments: greet <name> [times]. */
SHELL_CMD_ARG_REGISTER(greet, NULL, "Greet someone: greet <name> [times]", cmd_greet, 2, 1);

int main(void)
{
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("The LED's GPIO controller is not ready");
		return 0;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	/*
	 * main() returns, and the shell keeps running: it has a thread of its own, started before
	 * main() by the shell subsystem.
	 */
	LOG_INF("Type 'help', or press Tab twice to list the commands");
	return 0;
}
