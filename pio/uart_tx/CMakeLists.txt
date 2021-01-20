add_executable(pio_uart_tx)

pico_generate_pio_header(pio_uart_tx ${CMAKE_CURRENT_LIST_DIR}/uart_tx.pio)

target_sources(pio_uart_tx PRIVATE uart_tx.c)

target_link_libraries(pio_uart_tx PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_uart_tx)

# add url via pico_set_program_url
example_auto_set_url(pio_uart_tx)
