# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kconfig)

target_sources(app PRIVATE src/main.c)

# Kconfig has run by now, so every option is a CMake variable too. This file is only compiled
# into the application when CONFIG_APP_SHOUT=y.
target_sources_ifdef(CONFIG_APP_SHOUT app PRIVATE src/shout.c)
