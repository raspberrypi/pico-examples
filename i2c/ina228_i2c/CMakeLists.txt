add_executable(ina228_i2c ina228_i2c.c)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(ina228_i2c
    pico_stdlib
    hardware_i2c
)
# Wait at most 3s for stdio_usb to be ready
target_compile_definitions(ina228_i2c PRIVATE
   PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
)

# create map/bin/hex file etc.
pico_add_extra_outputs(ina228_i2c)

# add url via pico_set_program_url
example_auto_set_url(ina228_i2c)

pico_enable_stdio_usb(ina228_i2c 1)
pico_enable_stdio_uart(ina228_i2c 1)
