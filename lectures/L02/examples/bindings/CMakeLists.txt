# SPDX-License-Identifier: MIT

# Two files beside this one are found by convention, with no CMake line: app.overlay, and the
# binding in dts/bindings/, because the application directory is always searched for bindings.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bindings)

target_sources(app PRIVATE src/main.c)
