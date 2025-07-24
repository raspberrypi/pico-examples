add_executable(uart_boot
    uart_boot.c
    )

# pull in common dependencies
target_link_libraries(uart_boot pico_stdlib hardware_flash)

# add partition table
pico_embed_pt_in_binary(uart_boot ${CMAKE_CURRENT_LIST_DIR}/uart-pt.json)

# create absolute UF2, as it's a bootloader so shouldn't go in a partition
pico_set_uf2_family(uart_boot "absolute")

# create map/bin/hex file etc.
pico_add_extra_outputs(uart_boot)

# add url via pico_set_program_url
example_auto_set_url(uart_boot)


# Create separate binary to be loaded onto other device
add_executable(uart_binary
    uart_binary.c
    )

# pull in common dependencies
target_link_libraries(uart_binary pico_stdlib)

pico_set_binary_type(uart_binary no_flash)

# package uf2 in flash
pico_package_uf2_output(uart_binary)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(uart_binary)

# call pico_set_program_url to set path to example on github, so users can find the source for an example via picotool
example_auto_set_url(uart_binary)
