if (TARGET pico_mbedtls)
    # note uart bootloader doesn't use Mbed TLS, but it requires picotool with Mbed TLS, so the above check is a good proxy
    add_subdirectory_exclude_platforms(uart host rp2040)
    add_subdirectory_exclude_platforms(encrypted host rp2040 rp2350-riscv)
else()
    # Assume picotool has no signing support, if no pico_mbedtls available
    message("Skipping uart bootloader and encrypted bootloader examples as Mbed TLS is not available")
endif ()
