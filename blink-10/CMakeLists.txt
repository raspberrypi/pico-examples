add_executable(blink-10
        blink-10.c
        )

# pull in common dependencies
target_link_libraries(blink-10 pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(blink-10)

# add url via pico_set_program_url
example_auto_set_url(blink-10)
