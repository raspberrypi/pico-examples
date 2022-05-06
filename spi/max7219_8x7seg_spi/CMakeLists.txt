add_executable(max7219_8x7seg_spi
        max7219_8x7seg_spi.c
        )

# pull in common dependencies and additional spi hardware support
target_link_libraries(max7219_8x7seg_spi pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(max7219_8x7seg_spi)

# add url via pico_set_program_url
example_auto_set_url(max7219_8x7seg_spi)
