add_executable(pio_quadrature_encoder)

pico_generate_pio_header(pio_quadrature_encoder ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)

target_sources(pio_quadrature_encoder PRIVATE quadrature_encoder.c)

target_link_libraries(pio_quadrature_encoder PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        )

pico_enable_stdio_usb(pio_quadrature_encoder 1)

pico_add_extra_outputs(pio_quadrature_encoder)

# add url via pico_set_program_url
example_auto_set_url(pio_quadrature_encoder)
