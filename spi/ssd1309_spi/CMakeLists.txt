add_executable(ssd1309_spi
        ssd1309_spi.c
        )

# pull in common dependencies and additional spi hardware support
target_link_libraries(ssd1309_spi pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(ssd1309_spi)

# add url via pico_set_program_url
example_auto_set_url(ssd1309_spi)
