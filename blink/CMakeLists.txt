add_executable(blink
        blink.c
        )

target_compile_definitions(blink PUBLIC PICO_DEFAULT_LED_PIN=0)

# pull in common dependencies
target_link_libraries(blink pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(blink)

# add url via pico_set_program_url
example_auto_set_url(blink)
