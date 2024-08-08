add_executable(multicore_doorbell
        multicore_doorbell.c
        )
target_link_libraries(multicore_doorbell
        pico_stdlib
        pico_multicore)
pico_add_extra_outputs(multicore_doorbell)
example_auto_set_url(multicore_doorbell)
