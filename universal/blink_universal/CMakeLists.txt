if (NOT PICO_BOARD STREQUAL "universal")
    message(FATAL_ERROR "PICO_BOARD for blink_universal must be set to 'universal', not '${PICO_BOARD}'")
    return()
endif()

add_executable(blink_universal
    blink_universal.c
    )

# pull in common dependencies
target_link_libraries(blink_universal pico_stdlib hardware_adc)
target_link_libraries(blink_universal pico_cyw43_arch_none)

# create map/bin/hex file etc.
pico_add_extra_outputs(blink_universal)

# add url via pico_set_program_url
example_auto_set_url(blink_universal)
