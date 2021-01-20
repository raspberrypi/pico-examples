add_executable(hello_dma
        hello_dma.c
        )

target_link_libraries(hello_dma pico_stdlib hardware_dma)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_dma)

# add url via pico_set_program_url
example_auto_set_url(hello_dma)
