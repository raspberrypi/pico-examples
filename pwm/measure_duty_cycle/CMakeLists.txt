add_executable(pwm_measure_duty_cycle
        measure_duty_cycle.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(pwm_measure_duty_cycle pico_stdlib hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(pwm_measure_duty_cycle)

# add url via pico_set_program_url
example_auto_set_url(pwm_measure_duty_cycle)
