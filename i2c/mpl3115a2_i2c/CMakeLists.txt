add_executable(mpl3115a2_i2c
        mpl3115a2_i2c.c
        )

# pull in common dependencies and additional i2c hardware support
target_link_libraries(mpl3115a2_i2c pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(mpl3115a2_i2c)

# add url via pico_set_program_url
example_auto_set_url(mpl3115a2_i2c)
