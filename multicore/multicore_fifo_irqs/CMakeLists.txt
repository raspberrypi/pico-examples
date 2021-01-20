add_executable(multicore_fifo_irqs
        multicore_fifo_irqs.c
        )

target_link_libraries(multicore_fifo_irqs
        pico_multicore
        pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(multicore_fifo_irqs)

# add url via pico_set_program_url
example_auto_set_url(multicore_fifo_irqs)