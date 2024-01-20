add_executable(pio_ppm)

pico_generate_pio_header(pio_ppm ${CMAKE_CURRENT_LIST_DIR}/ppm.pio)

target_sources(pio_ppm PRIVATE ppm.c)

target_link_libraries(pio_ppm PRIVATE pico_stdlib hardware_pio hardware_clocks)
pico_add_extra_outputs(pio_ppm)

# add url via pico_set_program_url
example_auto_set_url(pio_ppm)