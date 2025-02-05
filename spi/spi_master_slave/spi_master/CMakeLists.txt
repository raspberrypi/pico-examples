add_executable(spi_master
        spi_master.c
        )

# Pull in basic dependencies
target_link_libraries(spi_master pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(spi_master)

# add url via pico_set_program_url
example_auto_set_url(spi_master)
