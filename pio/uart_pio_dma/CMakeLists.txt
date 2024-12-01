add_executable(uart_pio_dma)
pico_generate_pio_header(uart_pio_dma ${CMAKE_CURRENT_LIST_DIR}/../uart_rx/uart_rx.pio)
pico_generate_pio_header(uart_pio_dma ${CMAKE_CURRENT_LIST_DIR}/../uart_tx/uart_tx.pio)
target_sources(uart_pio_dma PRIVATE uart_pio_dma.c)
target_link_libraries(uart_pio_dma PRIVATE
        pico_stdlib
        hardware_pio
        hardware_dma
        )
pico_add_extra_outputs(uart_pio_dma)
example_auto_set_url(uart_pio_dma)
