add_executable(hello_gpout
        hello_gpout.c
        )

# pull in common dependencies
target_link_libraries(hello_gpout pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_gpout)

# add url via pico_set_program_url
example_auto_set_url(hello_gpout)
