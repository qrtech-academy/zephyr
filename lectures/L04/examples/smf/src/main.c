/*
 * SPDX-License-Identifier: MIT
 *
 * A heater controller written with the State Machine Framework, driven by events.
 */

#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

/* Each event is one bit, so a k_event can hold several that have not been handled yet. */
#define EVENT_START        BIT(0)
#define EVENT_STOP         BIT(1)
#define EVENT_TEMP_REACHED BIT(2)
#define EVENT_TEMP_DROPPED BIT(3)
#define EVENT_FAULT        BIT(4)
#define EVENT_RESET        BIT(5)
#define ALL_EVENTS         (BIT(6) - 1)

static const char *const event_names[] = {"START",        "STOP",  "TEMP_REACHED",
					  "TEMP_DROPPED", "FAULT", "RESET"};

K_EVENT_DEFINE(heater_events);

/*
 * The object the state machine works on. SMF requires its context to be the first member, so that
 * a pointer to the object is also a pointer to the context.
 */
struct heater {
	struct smf_ctx ctx;
	uint32_t event;
};

enum heater_state {
	OPERATIONAL,
	IDLE,
	HEATING,
	HOLD,
	FAULT,
};

static const struct smf_state states[];

static void trace(const char *state, const char *what)
{
	printk("[%4lld ms] %-11s %s\n", k_uptime_get(), state, what);
}

static const char *event_name(uint32_t event)
{
	return event_names[u32_count_trailing_zeros(event)];
}

/*
 * OPERATIONAL is the parent of IDLE, HEATING and HOLD. It handles what all three have in common:
 * a fault, from whichever of them the heater is in.
 */
static void operational_entry(void *obj)
{
	trace("OPERATIONAL", "entry");
}

static enum smf_state_result operational_run(void *obj)
{
	struct heater *heater = obj;

	if (heater->event == EVENT_FAULT) {
		smf_set_state(SMF_CTX(heater), &states[FAULT]);
	} else {
		trace("OPERATIONAL", "ignores the event");
	}
	return SMF_EVENT_HANDLED;
}

static void operational_exit(void *obj)
{
	trace("OPERATIONAL", "exit");
}

static void idle_entry(void *obj)
{
	trace("IDLE", "entry: heater off");
}

static enum smf_state_result idle_run(void *obj)
{
	struct heater *heater = obj;

	if (heater->event == EVENT_START) {
		smf_set_state(SMF_CTX(heater), &states[HEATING]);
		return SMF_EVENT_HANDLED;
	}
	/* Not an event IDLE knows: let the parent decide. */
	return SMF_EVENT_PROPAGATE;
}

static void heating_entry(void *obj)
{
	trace("HEATING", "entry: heater on");
}

static enum smf_state_result heating_run(void *obj)
{
	struct heater *heater = obj;

	switch (heater->event) {
	case EVENT_TEMP_REACHED:
		smf_set_state(SMF_CTX(heater), &states[HOLD]);
		return SMF_EVENT_HANDLED;
	case EVENT_STOP:
		smf_set_state(SMF_CTX(heater), &states[IDLE]);
		return SMF_EVENT_HANDLED;
	default:
		return SMF_EVENT_PROPAGATE;
	}
}

static void heating_exit(void *obj)
{
	trace("HEATING", "exit: heater off");
}

static void hold_entry(void *obj)
{
	trace("HOLD", "entry: holding the temperature");
}

static enum smf_state_result hold_run(void *obj)
{
	struct heater *heater = obj;

	switch (heater->event) {
	case EVENT_TEMP_DROPPED:
		smf_set_state(SMF_CTX(heater), &states[HEATING]);
		return SMF_EVENT_HANDLED;
	case EVENT_STOP:
		smf_set_state(SMF_CTX(heater), &states[IDLE]);
		return SMF_EVENT_HANDLED;
	default:
		return SMF_EVENT_PROPAGATE;
	}
}

/* FAULT is outside OPERATIONAL, so entering it exits OPERATIONAL, and only RESET leaves it. */
static void fault_entry(void *obj)
{
	trace("FAULT", "entry: heater off until reset");
}

static enum smf_state_result fault_run(void *obj)
{
	struct heater *heater = obj;

	if (heater->event == EVENT_RESET) {
		smf_set_state(SMF_CTX(heater), &states[IDLE]);
	} else {
		trace("FAULT", "ignores the event");
	}
	return SMF_EVENT_HANDLED;
}

static void fault_exit(void *obj)
{
	trace("FAULT", "exit");
}

/* The whole state machine, as data: entry, run and exit actions, and each state's parent. */
static const struct smf_state states[] = {
	[OPERATIONAL] =
		SMF_CREATE_STATE(operational_entry, operational_run, operational_exit, NULL, NULL),
	[IDLE] = SMF_CREATE_STATE(idle_entry, idle_run, NULL, &states[OPERATIONAL], NULL),
	[HEATING] = SMF_CREATE_STATE(heating_entry, heating_run, heating_exit, &states[OPERATIONAL],
				     NULL),
	[HOLD] = SMF_CREATE_STATE(hold_entry, hold_run, NULL, &states[OPERATIONAL], NULL),
	[FAULT] = SMF_CREATE_STATE(fault_entry, fault_run, fault_exit, NULL, NULL),
};

static void heater_main(void)
{
	struct heater heater = {0};

	/* Entering IDLE first enters its parent, OPERATIONAL. */
	smf_set_initial(SMF_CTX(&heater), &states[IDLE]);

	while (true) {
		/* Wait for any events, and clear the ones returned, in one atomic step. */
		uint32_t events = k_event_wait_safe(&heater_events, ALL_EVENTS, false, K_FOREVER);

		/* Run the state machine once per event, lowest bit first. */
		while (events != 0) {
			heater.event = BIT(u32_count_trailing_zeros(events));
			events &= ~heater.event;
			printk("[%4lld ms] event %s\n", k_uptime_get(), event_name(heater.event));
			smf_run_state(SMF_CTX(&heater));
		}
	}
}

/* Stands in for the rest of the system: a user, a temperature sensor, a fault detector. */
static void stimulus_main(void)
{
	static const struct {
		int delay_ms;
		uint32_t event;
	} script[] = {
		{100, EVENT_START},        {300, EVENT_TEMP_REACHED}, {200, EVENT_TEMP_DROPPED},
		{100, EVENT_TEMP_REACHED}, {200, EVENT_FAULT},        {100, EVENT_START},
		{100, EVENT_RESET},        {100, EVENT_STOP},
	};

	for (size_t i = 0; i < ARRAY_SIZE(script); i++) {
		k_msleep(script[i].delay_ms);
		k_event_post(&heater_events, script[i].event);
	}
}

K_THREAD_DEFINE(heater_thread, 1024, heater_main, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(stimulus_thread, 1024, stimulus_main, NULL, NULL, NULL, 6, 0, 0);
