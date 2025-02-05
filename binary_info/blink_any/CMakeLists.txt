if (NOT PICO_CYW43_SUPPORTED)
    message("Only building blink_any for non W boards as PICO_CYW43_SUPPORTED is not set")
endif()

add_executable(blink_any
    blink_any.c
    )

# pull in common dependencies
target_link_libraries(blink_any pico_stdlib)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(blink_any pico_cyw43_arch_none)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(blink_any)

# add url via pico_set_program_url
example_auto_set_url(blink_any)
