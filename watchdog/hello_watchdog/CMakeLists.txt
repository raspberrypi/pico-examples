add_executable(hello_watchdog
        hello_watchdog.c
        )

target_link_libraries(hello_watchdog pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_watchdog)

# add url via pico_set_program_url
example_auto_set_url(hello_watchdog)
