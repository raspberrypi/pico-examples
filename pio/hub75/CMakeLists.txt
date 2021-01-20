add_executable(pio_hub75)

pico_generate_pio_header(pio_hub75 ${CMAKE_CURRENT_LIST_DIR}/hub75.pio)

target_sources(pio_hub75 PRIVATE hub75.c)

target_compile_definitions(pio_hub75 PRIVATE
	PICO_DEFAULT_UART_TX_PIN=28
	PICO_DEFAULT_UART_RX_PIN=29
)

target_link_libraries(pio_hub75 PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_hub75)

# add url via pico_set_program_url
example_auto_set_url(pio_hub75)
