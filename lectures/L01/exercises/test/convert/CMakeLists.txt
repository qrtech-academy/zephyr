# SPDX-License-Identifier: MIT
#
# A ztest application for Exercise Set 3's celsius_to_fahrenheit(). ci/suite.py passes the set's
# directory, in your workspace or in the solutions, as EXERCISE_DIR, and the tests are compiled
# together with the convert.c found there.

cmake_minimum_required(VERSION 3.20.0)

if(NOT DEFINED EXERCISE_DIR)
  message(FATAL_ERROR "Run this suite with 'make test', which sets -DEXERCISE_DIR.")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(l01_convert_test)

target_include_directories(app PRIVATE ${EXERCISE_DIR}/src)
target_sources(app PRIVATE src/main.c ${EXERCISE_DIR}/src/convert.c)
