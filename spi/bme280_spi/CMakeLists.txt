add_executable(bme280_spi
        bme280_spi.c
        )

# pull in common dependencies and additional spi hardware support
target_link_libraries(bme280_spi pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(bme280_spi)

# add url via pico_set_program_url
example_auto_set_url(bme280_spi)
