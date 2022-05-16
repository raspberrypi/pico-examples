add_executable(hello_resus
        hello_resus.c
        )

# pull in common dependencies
target_link_libraries(hello_resus pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_resus)

# add url via pico_set_program_url
example_auto_set_url(hello_resus)
