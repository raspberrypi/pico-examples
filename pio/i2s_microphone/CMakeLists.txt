add_executable(i2s_microphone)

pico_generate_pio_header(i2s_microphone ${CMAKE_CURRENT_LIST_DIR}/i2s_microphone.pio)

target_sources(i2s_microphone PRIVATE i2s_microphone.c)

target_link_libraries(i2s_microphone PRIVATE
        pico_stdlib
        hardware_pio
        )

pico_add_extra_outputs(i2s_microphone)

# add url via pico_set_program_url
example_auto_set_url(i2s_microphone)
