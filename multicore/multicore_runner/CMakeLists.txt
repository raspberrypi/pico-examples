add_executable(multicore_runner
        multicore_runner.c
        )

target_link_libraries(multicore_runner
        pico_multicore
        pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(multicore_runner)

# add url via pico_set_program_url
example_auto_set_url(multicore_runner)