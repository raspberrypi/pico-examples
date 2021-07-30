add_executable(spi_flash
        spi_flash.c
        )

# Pull in basic dependencies
target_link_libraries(spi_flash pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(spi_flash)

# add url via pico_set_program_url
example_auto_set_url(spi_flash)
