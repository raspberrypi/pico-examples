add_executable(digital_display
        digital_display.c
        )

target_compile_definitions(digital_display PUBLIC PICO_DEFAULT_LED_PIN=0)

# pull in common dependencies
target_link_libraries(digital_display pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(digital_display)

# add url via pico_set_program_url
example_auto_set_url(digital_display)
