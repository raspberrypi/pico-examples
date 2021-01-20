add_executable(dma_channel_irq
        channel_irq.c
        )

pico_generate_pio_header(dma_channel_irq ${CMAKE_CURRENT_LIST_DIR}/pio_serialiser.pio)

target_link_libraries(dma_channel_irq
		pico_stdlib
        hardware_dma
        hardware_irq
        hardware_pio
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(dma_channel_irq)

# add url via pico_set_program_url
example_auto_set_url(dma_channel_irq)
