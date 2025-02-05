add_executable(joystick_display
        joystick_display.c
        )

target_link_libraries(joystick_display pico_stdlib hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(joystick_display)

# add url via pico_set_program_url
example_auto_set_url(joystick_display)
