if (TARGET hardware_reset)
    add_executable(hello_reset
            hello_reset.c
            )

    # pull in common dependencies
    target_link_libraries(hello_reset pico_stdlib)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(hello_reset)

    # add url via pico_set_program_url
    example_auto_set_url(hello_reset)
endif ()
