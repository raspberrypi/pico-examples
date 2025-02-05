add_executable(pio_apa102)

pico_generate_pio_header(pio_apa102 ${CMAKE_CURRENT_LIST_DIR}/apa102.pio)

target_sources(pio_apa102 PRIVATE apa102.c)

target_link_libraries(pio_apa102 PRIVATE
        pico_stdlib
        hardware_pio
        )

pico_add_extra_outputs(pio_apa102)

# add url via pico_set_program_url
example_auto_set_url(pio_apa102)
