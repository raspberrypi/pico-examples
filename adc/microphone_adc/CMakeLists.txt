add_executable(microphone_adc
        microphone_adc.c
        )

# pull in common dependencies and adc hardware support
target_link_libraries(microphone_adc pico_stdlib hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(microphone_adc)

# add url via pico_set_program_url
example_auto_set_url(microphone_adc)
