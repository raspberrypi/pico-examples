add_executable(pio_onewire)

target_sources(pio_onewire PRIVATE onewire.c)

add_subdirectory(onewire_library)

target_link_libraries(pio_onewire PRIVATE
    pico_stdlib
    hardware_pio
    onewire_library)

pico_add_extra_outputs(pio_onewire)

# add url via pico_set_program_url
example_auto_set_url(pio_onewire)
