add_executable(pio_rs422_tx)

pico_generate_pio_header(pio_rs422_tx ${CMAKE_CURRENT_LIST_DIR}/rs422_tx.pio)

target_sources(pio_rs422_tx PRIVATE rs422_tx.c)

target_link_libraries(pio_rs422_tx PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_rs422_tx)

# add url via pico_set_program_url
example_auto_set_url(pio_rs422_tx)
