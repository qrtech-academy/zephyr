# SPDX-License-Identifier: MIT

# A library of its own, linked into the Zephyr image like any in-tree driver.
zephyr_library()
zephyr_library_sources(thermo.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_THERMO thermo_emul.c)
