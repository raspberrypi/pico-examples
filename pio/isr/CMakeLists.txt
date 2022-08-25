add_executable(isr)

pico_generate_pio_header(isr ${CMAKE_CURRENT_LIST_DIR}/simply_isr.pio)

target_sources(isr PRIVATE main.c)

pico_enable_stdio_usb(isr 1)
pico_enable_stdio_uart(isr 0)

target_link_libraries(isr PRIVATE
        pico_stdlib
        hardware_pio
        )

pico_add_extra_outputs(isr)

# add url via pico_set_program_url
example_auto_set_url(isr)
