add_executable(onboard_temperature onboard_temperature.c)

target_link_libraries(onboard_temperature pico_stdlib hardware_adc)

# enable uart output, disable usb output
pico_enable_stdio_uart(onboard_temperature 1)
pico_enable_stdio_usb(onboard_temperature 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(onboard_temperature)

# add url via pico_set_program_url
example_auto_set_url(onboard_temperature)

