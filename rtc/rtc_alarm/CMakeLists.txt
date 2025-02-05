add_executable(rtc_alarm
        rtc_alarm.c
        )

# pull in common dependencies and additional rtc hardware support
target_link_libraries(rtc_alarm pico_stdlib hardware_rtc)

# create map/bin/hex file etc.
pico_add_extra_outputs(rtc_alarm)

# add url via pico_set_program_url
example_auto_set_url(rtc_alarm)
