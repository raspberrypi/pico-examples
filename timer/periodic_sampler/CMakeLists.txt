if (NOT PICO_TIME_NO_ALARM_SUPPORT)
    add_executable(periodic_sampler
            periodic_sampler.c
            )

    # pull in common dependencies
    target_link_libraries(periodic_sampler pico_stdlib)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(periodic_sampler)

    # add url via pico_set_program_url
    example_auto_set_url(periodic_sampler)
endif()
