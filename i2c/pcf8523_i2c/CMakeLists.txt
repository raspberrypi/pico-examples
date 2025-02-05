add_executable(pcf8523_i2c
        pcf8523_i2c.c
        )

# pull in common dependencies and additional i2c hardware support
target_link_libraries(pcf8523_i2c pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(pcf8523_i2c)

# add url via pico_set_program_url
example_auto_set_url(pcf8523_i2c)
