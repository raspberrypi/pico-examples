add_executable(flash_nuke
        nuke.c
        )

target_link_libraries(flash_nuke
        pico_stdlib
        hardware_flash
        )

# It doesn't make sense to run this program from flash. Always build a
# RAM-only binary.
pico_set_binary_type(flash_nuke no_flash)

pico_add_extra_outputs(flash_nuke)

# add url via pico_set_program_url
example_auto_set_url(flash_nuke)

