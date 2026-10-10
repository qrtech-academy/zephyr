# SPDX-License-Identifier: MIT

# Every Zephyr application starts with these three lines. find_package(Zephyr) is where the build
# system takes over: it reads the board, runs Kconfig and devicetree, and creates the `app` target
# that your sources are added to.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello)

target_sources(app PRIVATE src/main.c)
