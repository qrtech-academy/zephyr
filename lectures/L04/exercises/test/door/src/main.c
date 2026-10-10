/*
 * SPDX-License-Identifier: MIT
 *
 * Tests for Exercise Set 4's garage door state machine.
 */

#include <zephyr/ztest.h>

#include "door.h"

/* Check the state, the motor, and how many times the motor has been started, in one line. */
#define zassert_door(state, motor, starts)                                                         \
	do {                                                                                       \
		zassert_equal(door_get_state(), state, "state is %d, not %d", door_get_state(),    \
			      state);                                                              \
		zassert_equal(door_motor_on(), motor, "the motor should be %s",                    \
			      (motor) ? "on" : "off");                                             \
		zassert_equal(door_motor_starts(), starts,                                         \
			      "the motor was started %u times, not %u", door_motor_starts(),       \
			      starts);                                                             \
	} while (0)

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	door_init();
}

ZTEST_SUITE(door, NULL, NULL, before, NULL, NULL);

ZTEST(door, test_starts_closed)
{
	zassert_door(DOOR_STATE_CLOSED, false, 0);
}

ZTEST(door, test_opens_and_closes)
{
	door_handle(DOOR_EVENT_BUTTON);
	zassert_door(DOOR_STATE_OPENING, true, 1);
	door_handle(DOOR_EVENT_OPENED);
	zassert_door(DOOR_STATE_OPEN, false, 1);
	door_handle(DOOR_EVENT_BUTTON);
	zassert_door(DOOR_STATE_CLOSING, true, 2);
	door_handle(DOOR_EVENT_CLOSED);
	zassert_door(DOOR_STATE_CLOSED, false, 2);
}

ZTEST(door, test_button_stops_the_door_while_opening)
{
	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_BUTTON);
	zassert_door(DOOR_STATE_STOPPED, false, 1);
}

ZTEST(door, test_button_stops_the_door_while_closing)
{
	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_OPENED);
	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_BUTTON);
	zassert_door(DOOR_STATE_STOPPED, false, 2);
}

ZTEST(door, test_button_closes_a_stopped_door)
{
	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_BUTTON);
	zassert_door(DOOR_STATE_CLOSING, true, 2);
}

ZTEST(door, test_obstacle_reverses_a_closing_door_without_restarting_the_motor)
{
	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_OPENED);
	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_OBSTACLE);
	zassert_door(DOOR_STATE_OPENING, true, 2);
}

ZTEST(door, test_obstacle_is_ignored_while_opening)
{
	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_OBSTACLE);
	zassert_door(DOOR_STATE_OPENING, true, 1);
}

ZTEST(door, test_end_switches_are_ignored_when_they_do_not_apply)
{
	door_handle(DOOR_EVENT_OPENED);
	door_handle(DOOR_EVENT_CLOSED);
	door_handle(DOOR_EVENT_OBSTACLE);
	zassert_door(DOOR_STATE_CLOSED, false, 0);

	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_CLOSED);
	zassert_door(DOOR_STATE_OPENING, true, 1);

	door_handle(DOOR_EVENT_OPENED);
	door_handle(DOOR_EVENT_OPENED);
	door_handle(DOOR_EVENT_CLOSED);
	door_handle(DOOR_EVENT_OBSTACLE);
	zassert_door(DOOR_STATE_OPEN, false, 1);

	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_OPENED);
	zassert_door(DOOR_STATE_CLOSING, true, 2);

	door_handle(DOOR_EVENT_BUTTON);
	door_handle(DOOR_EVENT_OPENED);
	door_handle(DOOR_EVENT_CLOSED);
	door_handle(DOOR_EVENT_OBSTACLE);
	zassert_door(DOOR_STATE_STOPPED, false, 2);
}

ZTEST(door, test_init_starts_over)
{
	door_handle(DOOR_EVENT_BUTTON);
	door_init();
	zassert_door(DOOR_STATE_CLOSED, false, 0);
}
