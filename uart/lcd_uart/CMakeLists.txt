add_executable(lcd_uart
        lcd_uart.c
        )

# pull in common dependencies and additional uart hardware support
target_link_libraries(lcd_uart pico_stdlib hardware_uart)

# create map/bin/hex file etc.
pico_add_extra_outputs(lcd_uart)

# add url via pico_set_program_url
example_auto_set_url(lcd_uart)
