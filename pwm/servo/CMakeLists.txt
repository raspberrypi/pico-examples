add_executable(servo main.c servo.c)

target_link_libraries(servo pico_stdlib hardware_pwm pico_bootrom hardware_clocks)

pico_add_extra_outputs(servo)