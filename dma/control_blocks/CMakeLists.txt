add_executable(dma_control_blocks
        control_blocks.c
        )

target_link_libraries(dma_control_blocks pico_stdlib hardware_dma)

# create map/bin/hex file etc.
pico_add_extra_outputs(dma_control_blocks)

# add url via pico_set_program_url
example_auto_set_url(dma_control_blocks)
