add_executable(pio_isr)

pico_generate_pio_header(pio_isr ${CMAKE_CURRENT_LIST_DIR}/simple_isr.pio)

target_sources(pio_isr PRIVATE main.c)

pico_enable_stdio_usb(pio_isr 1)
pico_enable_stdio_uart(pio_isr 0)

target_link_libraries(pio_isr PRIVATE
        pico_stdlib
        hardware_pio
        )

pico_add_extra_outputs(pio_isr)

# add url via pico_set_program_url
example_auto_set_url(pio_isr)
