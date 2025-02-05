add_executable(multicore_runner_queue
        multicore_runner_queue.c
        )

target_link_libraries(multicore_runner_queue
        pico_multicore
        pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(multicore_runner_queue)

# add url via pico_set_program_url
example_auto_set_url(multicore_runner_queue)