add_executable(mpu9250_spi
        mpu9250_spi.c
        )

# pull in common dependencies and additional spi hardware support
target_link_libraries(mpu9250_spi pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(mpu9250_spi)

# add url via pico_set_program_url
example_auto_set_url(mpu9250_spi)
