/*
 * SPDX-License-Identifier: MIT
 *
 * Build a table of blink patterns at compile time from the devicetree, and play them.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bindings, LOG_LEVEL_INF);

/*
 * The compatible "qacademy,blink-pattern" written as a C token: lowercase, with every character
 * that is not a letter or digit replaced by an underscore.
 */
#define PATTERN_COMPAT qacademy_blink_pattern

struct pattern {
	const char *node;
	const char *name;
	struct gpio_dt_spec led;
	const uint16_t *delays_ms;
	size_t steps;
	uint32_t repeat;
};

/*
 * Each node's delays-ms array becomes a static array of its own. DT_PROP() of an array property
 * expands to a brace-enclosed initializer, { 100, 100, ... }, and node_id is a unique C token
 * (for example DT_N_S_pattern_sos), so it can be pasted into a name.
 */
#define PATTERN_DELAYS(node_id)                                                                    \
	static const uint16_t delays_##node_id[] = DT_PROP(node_id, delays_ms);

DT_FOREACH_STATUS_OKAY(PATTERN_COMPAT, PATTERN_DELAYS)

/* One table entry per enabled node. Property names use underscores where the devicetree has '-'. */
#define PATTERN_ENTRY(node_id)                                                                     \
	{                                                                                          \
		.node = DT_NODE_FULL_NAME(node_id),                                                \
		.name = DT_PROP(node_id, pattern_name),                                            \
		.led = GPIO_DT_SPEC_GET(node_id, led_gpios),                                       \
		.delays_ms = delays_##node_id,                                                     \
		.steps = DT_PROP_LEN(node_id, delays_ms),                                          \
		.repeat = DT_PROP(node_id, repeat),                                                \
	},

static const struct pattern patterns[] = {DT_FOREACH_STATUS_OKAY(PATTERN_COMPAT, PATTERN_ENTRY)};

/*
 * A single node can be named directly too, here by its label. Everything above is a compile-time
 * constant, so the build itself can check the devicetree against what the code assumes.
 */
BUILD_ASSERT(ARRAY_SIZE(patterns) == DT_NUM_INST_STATUS_OKAY(PATTERN_COMPAT));
BUILD_ASSERT(DT_PROP(DT_NODELABEL(heartbeat), repeat) == 2, "heartbeat should play twice");

static void play(const struct pattern *p)
{
	gpio_pin_configure_dt(&p->led, GPIO_OUTPUT_INACTIVE);

	for (uint32_t round = 0; round < p->repeat; round++) {
		for (size_t step = 0; step < p->steps; step++) {
			/* Even steps are on, odd steps are off. */
			gpio_pin_set_dt(&p->led, (step % 2) == 0);
			k_msleep(p->delays_ms[step]);
		}
	}

	gpio_pin_set_dt(&p->led, 0);
}

int main(void)
{
	LOG_INF("%zu pattern(s) enabled", ARRAY_SIZE(patterns));

	/* The disabled node is still in the devicetree; it is only left out of the loops. */
	LOG_INF("strobe exists: %s, enabled: %s",
		DT_NODE_EXISTS(DT_NODELABEL(strobe)) ? "yes" : "no",
		DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(strobe)) ? "yes" : "no");

	for (size_t i = 0; i < ARRAY_SIZE(patterns); i++) {
		const struct pattern *p = &patterns[i];
		uint32_t total_ms = 0;

		for (size_t step = 0; step < p->steps; step++) {
			total_ms += p->delays_ms[step];
		}

		LOG_INF("%s: \"%s\" on %s pin %u, %zu steps, %u ms, x%u", p->node, p->name,
			p->led.port->name, p->led.pin, p->steps, total_ms, p->repeat);
	}

	for (size_t i = 0; i < ARRAY_SIZE(patterns); i++) {
		play(&patterns[i]);
		LOG_INF("Played %s, uptime %lld ms", patterns[i].name, k_uptime_get());
	}

	return 0;
}
