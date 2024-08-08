add_executable(boot_info
        boot_info.c
        )
target_link_libraries(boot_info
        pico_stdlib
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(boot_info)

# add url via pico_set_program_url
example_auto_set_url(boot_info)
