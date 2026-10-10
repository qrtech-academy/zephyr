# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sockets)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_APP_SELF_TEST app PRIVATE src/self_test.c)
