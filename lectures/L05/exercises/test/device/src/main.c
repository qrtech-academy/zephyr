/*
 * SPDX-License-Identifier: MIT
 *
 * Tests for Exercise Set 1: the module, the binding, Kconfig, and the driver's init.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include <qacademy/lumo_emul.h>

static const struct device *const light = DEVICE_DT_GET(DT_NODELABEL(light));

ZTEST_SUITE(device, NULL, NULL, NULL, NULL, NULL);

ZTEST(device, test_kconfig_option_turns_itself_on)
{
	zassert_true(IS_ENABLED(CONFIG_LUMO),
		     "CONFIG_LUMO is not enabled, although prj.conf leaves it to the devicetree");
}

ZTEST(device, test_device_is_ready)
{
	zassert_true(device_is_ready(light), "init failed, although the part is genuine");
}

ZTEST(device, test_device_is_a_sensor)
{
	zassert_true(DEVICE_API_IS(sensor, light),
		     "the device's API is not a sensor API defined with DEVICE_API(sensor, ...)");
}

ZTEST(device, test_wrong_part_id_fails_init)
{
	const struct device *const broken = DEVICE_DT_GET(DT_NODELABEL(light_broken));
	const struct emul *const broken_emul = EMUL_DT_GET(DT_NODELABEL(light_broken));

	zassert_false(device_is_ready(broken), "a deferred device is ready before device_init()");

	lumo_emul_set_part_id(broken_emul, 0x00);
	zassert_equal(device_init(broken), -ENODEV, "init must return -ENODEV for a wrong PART_ID");
	zassert_false(device_is_ready(broken), "a part with the wrong PART_ID is ready");
}
