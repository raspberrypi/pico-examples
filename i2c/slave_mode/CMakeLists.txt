add_executable(i2c_slave_mode main.c)

target_link_libraries(i2c_slave_mode pico_stdlib hardware_gpio hardware_i2c)

pico_enable_stdio_usb(i2c_slave_mode 1)
pico_enable_stdio_uart(i2c_slave_mode 0)

pico_add_extra_outputs(i2c_slave_mode)
