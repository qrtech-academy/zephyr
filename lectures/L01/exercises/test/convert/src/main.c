/*
 * SPDX-License-Identifier: MIT
 *
 * Tests for Exercise Set 3's celsius_to_fahrenheit().
 */

#include <zephyr/ztest.h>

#include "convert.h"

ZTEST_SUITE(convert, NULL, NULL, NULL, NULL, NULL);

ZTEST(convert, test_fixed_points)
{
	zassert_equal(celsius_to_fahrenheit(0), 32, "0 C is 32 F");
	zassert_equal(celsius_to_fahrenheit(100), 212, "100 C is 212 F");
	zassert_equal(celsius_to_fahrenheit(-40), -40, "-40 C is -40 F");
}

ZTEST(convert, test_rounds_up)
{
	zassert_equal(celsius_to_fahrenheit(37), 99, "37 C is 98.6 F, which rounds to 99");
	zassert_equal(celsius_to_fahrenheit(21), 70, "21 C is 69.8 F, which rounds to 70");
}

ZTEST(convert, test_rounds_down)
{
	zassert_equal(celsius_to_fahrenheit(24), 75, "24 C is 75.2 F, which rounds to 75");
	zassert_equal(celsius_to_fahrenheit(1), 34, "1 C is 33.8 F, which rounds to 34");
	zassert_equal(celsius_to_fahrenheit(2), 36, "2 C is 35.6 F, which rounds to 36");
	zassert_equal(celsius_to_fahrenheit(3), 37, "3 C is 37.4 F, which rounds to 37");
}

ZTEST(convert, test_negative_rounds_to_nearest)
{
	/* Truncating division would give -1 and 1 here, rather than the nearest degree. */
	zassert_equal(celsius_to_fahrenheit(-18), 0, "-18 C is -0.4 F, which rounds to 0");
	zassert_equal(celsius_to_fahrenheit(-19), -2, "-19 C is -2.2 F, which rounds to -2");
	zassert_equal(celsius_to_fahrenheit(-21), -6, "-21 C is -5.8 F, which rounds to -6");
}
