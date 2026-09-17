# Add secure executable
add_executable(hello_trustzone_s secure.c)
target_link_libraries(hello_trustzone_s
        pico_stdlib
        hardware_watchdog
        pico_secure
)

# Packaged no_flash binary
pico_set_binary_type(hello_trustzone_s no_flash)
pico_package_uf2_output(hello_trustzone_s 0x10000000)
pico_add_extra_outputs(hello_trustzone_s)

# Add non-secure executable
add_executable(hello_trustzone_ns nonsecure.c)
target_link_libraries(hello_trustzone_ns
        pico_stdlib
)

# Set UF2 family manually
pico_set_uf2_family(hello_trustzone_ns "rp2350-arm-ns")

# Use stdio_usb
pico_enable_stdio_uart(hello_trustzone_ns 0)
pico_enable_stdio_usb(hello_trustzone_ns 1)

# Default flash binary
pico_add_extra_outputs(hello_trustzone_ns)

# Set security options
pico_set_security_options(hello_trustzone_s hello_trustzone_ns
    USER_IRQ USB
    NONSECURE_TIMER 1
)

# Set security ram split
pico_set_security_ram_split(hello_trustzone_s hello_trustzone_ns SIMPLE 64k 2k)

# Create concatenated UF2, for loading into the partition table using drag & drop (non-secure first)
pico_concatenate_uf2_outputs(hello_trustzone_drag_drop hello_trustzone_ns hello_trustzone_s)

# Create combined UF2 containing partition table, for loading using picotool or drag & drop
pico_combine_uf2_outputs(hello_trustzone_combined trustzone_pt
        trustzone_none hello_trustzone_s trustzone_none hello_trustzone_ns
        PARTITIONS 0 1 2 3)

# Create combined CMake target, for easy building
add_custom_target(hello_trustzone ALL
        DEPENDS hello_trustzone_combined hello_trustzone_drag_drop)


# no_flash example
# Add secure executable
add_executable(hello_trustzone_no_flash_s secure.c)
target_link_libraries(hello_trustzone_no_flash_s
        pico_stdlib
        hardware_watchdog
        pico_secure
)

# no_flash binary for secure side
pico_set_binary_type(hello_trustzone_no_flash_s no_flash)
pico_add_extra_outputs(hello_trustzone_no_flash_s)

# Add non-secure executable
add_executable(hello_trustzone_no_flash_ns nonsecure.c)
target_link_libraries(hello_trustzone_no_flash_ns
        pico_stdlib
)

# Set UF2 family manually
pico_set_uf2_family(hello_trustzone_no_flash_ns "rp2350-arm-ns")

# Use stdio_usb
pico_enable_stdio_uart(hello_trustzone_no_flash_ns 0)
pico_enable_stdio_usb(hello_trustzone_no_flash_ns 1)

# no_flash binary for non-secure side too
pico_set_binary_type(hello_trustzone_no_flash_ns no_flash)
pico_add_extra_outputs(hello_trustzone_no_flash_ns)

# Set security options
pico_set_security_options(hello_trustzone_no_flash_s hello_trustzone_no_flash_ns
    USER_IRQ USB
    NONSECURE_TIMER 1
)

# Set security ram split (with NO_FLASH)
pico_set_security_ram_split(hello_trustzone_no_flash_s hello_trustzone_no_flash_ns SIMPLE 64k 2k NO_FLASH)

# Create combined CMake target, for easy building
add_custom_target(hello_trustzone_no_flash ALL
        DEPENDS hello_trustzone_no_flash_s hello_trustzone_no_flash_ns)
