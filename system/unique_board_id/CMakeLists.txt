add_executable(unique_board_id
        unique_board_id.c
        )

target_link_libraries(unique_board_id
        pico_stdlib
        pico_unique_id
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(unique_board_id)

# add url via pico_set_program_url
example_auto_set_url(unique_board_id)

# enable both, usb output and uart output
pico_enable_stdio_usb(unique_board_id 1)
pico_enable_stdio_uart(unique_board_id 1)