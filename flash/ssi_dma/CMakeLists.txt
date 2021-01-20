add_executable(flash_ssi_dma
        flash_ssi_dma.c
        )

target_link_libraries(flash_ssi_dma
        pico_stdlib
        hardware_dma
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(flash_ssi_dma)

# add url via pico_set_program_url
example_auto_set_url(flash_ssi_dma)
