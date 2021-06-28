add_executable(bmp280_i2c
        bmp280_i2c.c
        )

# Pull in our get you started dependencies
target_link_libraries(bmp280_i2c pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(bmp280_i2c)

# add url via pico_set_program_url
example_auto_set_url(bmp280_i2c)