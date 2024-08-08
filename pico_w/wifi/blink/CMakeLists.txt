add_executable(picow_blink
        picow_blink.c
        )
target_link_libraries(picow_blink
        pico_stdlib              # for core functionality
        pico_cyw43_arch_none     # we need Wifi to access the GPIO, but we don't need anything else
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(picow_blink)

# add url via pico_set_program_url
example_auto_set_url(picow_blink)

# This version should behave exactly the same, but it runs the sys clock slowly.
add_executable(picow_blink_slow_clock
        picow_blink_slow_clock.c
        )
target_link_libraries(picow_blink_slow_clock
        pico_stdlib              # for core functionality
        pico_cyw43_arch_none     # we need Wifi to access the GPIO, but we don't need anything else
        hardware_clocks
        )
# This requires us to modify the pio divisor to successfully communicate with the cyw43 chip
target_compile_definitions(picow_blink_slow_clock PRIVATE
        CYW43_PIO_CLOCK_DIV_DYNAMIC=1
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(picow_blink_slow_clock)

# add url via pico_set_program_url
example_auto_set_url(picow_blink_slow_clock)
