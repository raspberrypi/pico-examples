add_executable(spi_slave
        spi_slave.c
        )

# Pull in basic dependencies
target_link_libraries(spi_slave pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(spi_slave)

# add url via pico_set_program_url
example_auto_set_url(spi_slave)
