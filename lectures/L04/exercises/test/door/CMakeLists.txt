# SPDX-License-Identifier: MIT
#
# A ztest application for Exercise Set 4's garage door. ci/suite.py passes the set's directory, in
# your workspace or in the solutions, as EXERCISE_DIR, and the tests are compiled together with the
# door.c found there.

cmake_minimum_required(VERSION 3.20.0)

if(NOT DEFINED EXERCISE_DIR)
  message(FATAL_ERROR "Run this suite with 'make test', which sets -DEXERCISE_DIR.")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(l04_door_test)

target_include_directories(app PRIVATE ${EXERCISE_DIR}/src)
target_sources(app PRIVATE src/main.c ${EXERCISE_DIR}/src/door.c)
