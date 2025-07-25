add_executable(pio_uart_dma)
pico_generate_pio_header(pio_uart_dma ${CMAKE_CURRENT_LIST_DIR}/../uart_rx/uart_rx.pio)
pico_generate_pio_header(pio_uart_dma ${CMAKE_CURRENT_LIST_DIR}/../uart_tx/uart_tx.pio)
target_sources(pio_uart_dma PRIVATE uart_dma.c)
target_link_libraries(pio_uart_dma PRIVATE
        pico_stdlib
        hardware_pio
        hardware_dma
        )
pico_add_extra_outputs(pio_uart_dma)
example_auto_set_url(pio_uart_dma)
