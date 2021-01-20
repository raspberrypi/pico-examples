if (PICO_ON_DEVICE)
    add_executable(timer_lowlevel
            timer_lowlevel.c)

    # Disable SDK alarm support for this lowlevel example
    set(PICO_TIME_DEFAULT_ALARM_POOL_DISABLED 1)

    target_link_libraries(timer_lowlevel pico_stdlib)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(timer_lowlevel)

    # add url via pico_set_program_url
    example_auto_set_url(timer_lowlevel)
endif ()