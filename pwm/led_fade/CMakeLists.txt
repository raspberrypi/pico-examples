add_executable(pwm_led_fade
        pwm_led_fade.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(pwm_led_fade pico_stdlib hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(pwm_led_fade)

# add url via pico_set_program_url
example_auto_set_url(pwm_led_fade)
