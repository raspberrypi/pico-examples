add_executable(flash_xip_stream
        flash_xip_stream.c
        )

target_link_libraries(flash_xip_stream
        pico_stdlib
        hardware_dma
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(flash_xip_stream)

# add url via pico_set_program_url
example_auto_set_url(flash_xip_stream)
