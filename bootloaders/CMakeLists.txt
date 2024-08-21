if (TARGET pico_mbedtls)
    add_subdirectory_exclude_platforms(encrypted host rp2040 rp2350-riscv)
else()
    # Assume picotool has no signing support, if no pico_mbedtls available
    message("Skipping encrypted bootloader example as pico_mbedtls unavailable")
endif ()
