add_executable(AM2302 AM2302.cpp AM2302-Sensor.cpp)

target_link_libraries(AM2302 pico_stdlib)

pico_add_extra_outputs(AM2302)

# add url via pico_set_program_url
example_auto_set_url(AM2302)