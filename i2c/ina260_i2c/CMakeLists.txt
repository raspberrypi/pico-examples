add_executable(ina260_i2c
    ina260_i2c.c
)
target_link_libraries(ina260_i2c
    pico_stdlib
    hardware_i2c
)
pico_add_extra_outputs(ina260_i2c)
