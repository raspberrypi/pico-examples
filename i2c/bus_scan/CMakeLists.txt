add_executable(i2c_bus_scan
        bus_scan.c
        )

# Pull in our (to be renamed) simple get you started dependencies
target_link_libraries(i2c_bus_scan pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(i2c_bus_scan)

# add url via pico_set_program_url
example_auto_set_url(i2c_bus_scan)
