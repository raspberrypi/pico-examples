add_executable(hello_adc
        hello_adc.c
        )

target_link_libraries(hello_adc pico_stdlib hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_adc)

# add url via pico_set_program_url
example_auto_set_url(hello_adc)
