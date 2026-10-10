# SPDX-License-Identifier: MIT

zephyr_include_directories(include)

if(CONFIG_EMUL_LUMO)
  zephyr_library()
  zephyr_library_sources(src/lumo_emul.c)
  zephyr_library_sources_ifdef(CONFIG_EMUL_LUMO_DAYLIGHT src/daylight.c)
endif()
