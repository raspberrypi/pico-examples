add_executable(pio_pwm)

pico_generate_pio_header(pio_pwm ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)

target_sources(pio_pwm PRIVATE pwm.c)

target_link_libraries(pio_pwm PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_pwm)

# add url via pico_set_program_url
example_auto_set_url(pio_pwm)