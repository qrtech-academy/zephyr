# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)

# The same module the application uses: the tests exercise the real driver, not a copy of it.
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../module)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(thermo_tests)

target_sources(app PRIVATE src/main.c)
