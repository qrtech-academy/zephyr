# SPDX-License-Identifier: MIT
#
# The module's build, included by Zephyr's own. By the time it runs, Kconfig has run, so the
# driver's directory is only added when CONFIG_THERMO=y.

# The emulator's header is public: tests and applications include it to control the emulator.
zephyr_include_directories(include)

add_subdirectory_ifdef(CONFIG_THERMO drivers/sensor/thermo)
