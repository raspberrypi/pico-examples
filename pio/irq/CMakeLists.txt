add_executable(pio_irq)

pico_generate_pio_header(pio_irq ${CMAKE_CURRENT_LIST_DIR}/simple_irq.pio)

target_sources(pio_irq PRIVATE main.c)

pico_enable_stdio_usb(pio_irq 1)
pico_enable_stdio_uart(pio_irq 0)

target_link_libraries(pio_irq PRIVATE
        pico_stdlib
        hardware_pio
        )

pico_add_extra_outputs(pio_irq)

# add url via pico_set_program_url
example_auto_set_url(pio_irq)
