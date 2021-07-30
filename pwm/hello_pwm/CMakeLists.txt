add_executable(hello_pwm
        hello_pwm.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(hello_pwm pico_stdlib hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_pwm)

# add url via pico_set_program_url
example_auto_set_url(hello_pwm)
