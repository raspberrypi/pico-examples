add_executable(sniff_crc
        sniff_crc.c
        )

target_link_libraries(sniff_crc pico_stdlib hardware_dma)

# create map/bin/hex file etc.
pico_add_extra_outputs(sniff_crc)

# add url via pico_set_program_url
example_auto_set_url(sniff_crc)
