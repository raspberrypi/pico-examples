add_executable(pio_blink)

# by default the header is generated into the build dir
pico_generate_pio_header(pio_blink ${CMAKE_CURRENT_LIST_DIR}/blink.pio)
# however, alternatively you can choose to generate it somewhere else (in this case in the source tree for check in)
#pico_generate_pio_header(pio_blink ${CMAKE_CURRENT_LIST_DIR}/blink.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_sources(pio_blink PRIVATE blink.c)

target_link_libraries(pio_blink PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_blink)

# add url via pico_set_program_url
example_auto_set_url(pio_blink)