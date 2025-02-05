add_executable(mcp9808_i2c
        mcp9808_i2c.c
        )

# pull in common dependencies and additional i2c hardware support
target_link_libraries(mcp9808_i2c pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(mcp9808_i2c)

# add url via pico_set_program_url
example_auto_set_url(mcp9808_i2c)