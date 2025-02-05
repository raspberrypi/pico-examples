add_executable(dht
        dht.c
        )

target_link_libraries(dht pico_stdlib)

pico_add_extra_outputs(dht)

# add url via pico_set_program_url
example_auto_set_url(dht)

