add_executable(rand
        rand.c
        )
target_link_libraries(rand
        pico_stdlib
        pico_rand
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(rand)

# add url via pico_set_program_url
example_auto_set_url(rand)
