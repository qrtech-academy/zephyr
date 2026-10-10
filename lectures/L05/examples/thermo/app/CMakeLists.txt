# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)

# The driver is not in Zephyr's tree but in a module beside this application. Naming the module
# before find_package(Zephyr) is enough for the build to include its CMakeLists.txt, its Kconfig
# and its devicetree bindings.
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../module)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(thermo_app)

target_sources(app PRIVATE src/main.c)

# The simulated physical world only exists where the sensor is emulated.
target_sources_ifdef(CONFIG_EMUL app PRIVATE src/world.c)
