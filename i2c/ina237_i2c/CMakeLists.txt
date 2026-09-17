add_executable(ina237_i2c
    ina237_i2c.c
)
target_link_libraries(ina237_i2c
    pico_stdlib
    pico_status_led
    hardware_i2c
    hardware_uart
)
# Wait at most 3s for stdio_usb to be ready
target_compile_definitions(ina237_i2c PRIVATE
   PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
)
pico_add_extra_outputs(ina237_i2c)
pico_enable_stdio_usb(ina237_i2c 1)
pico_enable_stdio_uart(ina237_i2c 1)
