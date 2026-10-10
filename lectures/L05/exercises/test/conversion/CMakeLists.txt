# SPDX-License-Identifier: MIT
#
# A ztest application for the Lumo driver in your module. ci/suite.py passes the exercise set's
# directory, in your workspace or in the solutions, as EXERCISE_DIR. That directory is your module;
# ../lumo_emul is the course's emulator, which plays the part on native_sim's I2C bus.

cmake_minimum_required(VERSION 3.20.0)

if(NOT DEFINED EXERCISE_DIR)
  message(FATAL_ERROR "Run this suite with 'make test', which sets -DEXERCISE_DIR.")
endif()

list(APPEND ZEPHYR_EXTRA_MODULES ${EXERCISE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../lumo_emul)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(l05_conversion_test)

target_sources(app PRIVATE src/main.c)
