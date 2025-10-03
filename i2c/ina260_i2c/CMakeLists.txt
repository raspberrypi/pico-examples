add_executable(ina260_i2c
    ina260_i2c.c
)
target_link_libraries(ina260_i2c
    pico_stdlib
    pico_status_led
    hardware_i2c
)
pico_add_extra_outputs(ina260_i2c)
pico_enable_stdio_usb(ina260_i2c 1)
pico_enable_stdio_uart(ina260_i2c 1)
