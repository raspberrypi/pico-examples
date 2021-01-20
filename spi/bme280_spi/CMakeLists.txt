add_executable(bme280_spi
        bme280_spi.c
        )

# Pull in our (to be renamed) simple get you started dependencies
target_link_libraries(bme280_spi pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(bme280_spi)

# add url via pico_set_program_url
example_auto_set_url(bme280_spi)
