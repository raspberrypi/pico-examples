add_executable(lcd_uart
        lcd_uart.c
        )

# pull in common dependencies and additional uart hardware support
target_link_libraries(lcd_uart pico_stdlib hardware_uart)

# enable usb output and uart output
# modify here as required
pico_enable_stdio_usb(lcd_uart 1)
pico_enable_stdio_uart(lcd_uart 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(lcd_uart)

# add url via pico_set_program_url
example_auto_set_url(lcd_uart)
