add_executable(low_power_dormant_timer
        low_power_dormant_timer.c
        )
target_link_libraries(low_power_dormant_timer PRIVATE
        pico_stdlib
        pico_low_power
        pico_status_led
        hardware_adc
)

# create map/bin/hex file etc.
pico_add_extra_outputs(low_power_dormant_timer)

# add url via pico_set_program_url
example_auto_set_url(low_power_dormant_timer)

pico_enable_stdio_usb(low_power_dormant_timer 0)
pico_enable_stdio_uart(low_power_dormant_timer 1)
