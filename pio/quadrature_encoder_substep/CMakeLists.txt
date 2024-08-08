add_executable(pio_quadrature_encoder_substep)

pico_generate_pio_header(pio_quadrature_encoder_substep ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder_substep.pio)

target_sources(pio_quadrature_encoder_substep PRIVATE quadrature_encoder_substep.c)

target_link_libraries(pio_quadrature_encoder_substep PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_pwm
        hardware_watchdog
        )

target_compile_definitions(pio_quadrature_encoder_substep PRIVATE PICO_STDIO_USB_DEFAULT_CRLF=0)
pico_enable_stdio_usb(pio_quadrature_encoder_substep 1)

pico_add_extra_outputs(pio_quadrature_encoder_substep)

# add url via pico_set_program_url
example_auto_set_url(pio_quadrature_encoder_substep)
