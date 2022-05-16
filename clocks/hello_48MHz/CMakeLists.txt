add_executable(hello_48MHz
        hello_48MHz.c
        )

# pull in common dependencies and additional clocks hardware support
target_link_libraries(hello_48MHz pico_stdlib hardware_clocks)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_48MHz)

# add url via pico_set_program_url
example_auto_set_url(hello_48MHz)
