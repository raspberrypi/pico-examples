add_executable(low_power_pstate_timer
    low_power_pstate_timer.c
    )
target_link_libraries(low_power_pstate_timer PRIVATE
    pico_stdlib
    pico_low_power
    pico_status_led
)
pico_add_extra_outputs(low_power_pstate_timer)
example_auto_set_url(low_power_pstate_timer)
pico_enable_stdio_usb(low_power_pstate_timer 0)
pico_enable_stdio_uart(low_power_pstate_timer 1)

add_executable(low_power_pstate_gpio
    low_power_pstate_gpio.c
    )
target_link_libraries(low_power_pstate_gpio PRIVATE
    pico_stdlib
    pico_low_power
    pico_status_led
)
pico_add_extra_outputs(low_power_pstate_gpio)
example_auto_set_url(low_power_pstate_gpio)
pico_enable_stdio_usb(low_power_pstate_gpio 0)
pico_enable_stdio_uart(low_power_pstate_gpio 1)
