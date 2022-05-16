add_executable(hello_rtc
        hello_rtc.c
        )

# pull in common dependencies and additional rtc hardware support
target_link_libraries(hello_rtc pico_stdlib hardware_rtc)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_rtc)

# add url via pico_set_program_url
example_auto_set_url(hello_rtc)
