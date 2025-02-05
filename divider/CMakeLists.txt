if (TARGET hardware_divider)
        add_executable(hello_divider
                hello_divider.c
                )

        # pull in common dependencies
        target_link_libraries(hello_divider pico_stdlib)

        # create map/bin/hex file etc.
        pico_add_extra_outputs(hello_divider)

        # add url via pico_set_program_url
        example_auto_set_url(hello_divider)
else()
        message("Skipping divider examples as hardware_divider is unavailable on this platform")
endif()
