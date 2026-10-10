# SPDX-License-Identifier: MIT

# app.overlay beside this file is picked up automatically: an application's devicetree overlay
# needs no CMake line, only the right file name.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(button)

target_sources(app PRIVATE src/main.c)
