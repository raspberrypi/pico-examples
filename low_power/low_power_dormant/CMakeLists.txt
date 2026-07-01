add_executable(low_power_dormant_timer
    low_power_dormant_timer.c
)
target_link_libraries(low_power_dormant_timer PRIVATE
    pico_stdlib
    pico_low_power
    pico_status_led
)
pico_add_extra_outputs(low_power_dormant_timer)
example_auto_set_url(low_power_dormant_timer)
pico_enable_stdio_usb(low_power_dormant_timer 0)
pico_enable_stdio_uart(low_power_dormant_timer 1)

add_executable(low_power_dormant_gpio
    low_power_dormant_gpio.c
)
target_link_libraries(low_power_dormant_gpio PRIVATE
    pico_stdlib
    pico_low_power
    pico_status_led
)
pico_add_extra_outputs(low_power_dormant_gpio)
example_auto_set_url(low_power_dormant_gpio)
pico_enable_stdio_usb(low_power_dormant_gpio 0)
pico_enable_stdio_uart(low_power_dormant_gpio 1)

# firmware for generating a clock on gp21 for running the dormant test on rp2040
# connect the clock out from gp21 to gp20 (or gp22) on the rp2040 running the dormant test
add_executable(low_power_clksrc
    low_power_clksrc.c
)
target_link_libraries(low_power_clksrc PRIVATE
    pico_stdlib
    pico_status_led
)
pico_add_extra_outputs(low_power_clksrc)
pico_enable_stdio_usb(low_power_clksrc 0)
pico_enable_stdio_uart(low_power_clksrc 1)
