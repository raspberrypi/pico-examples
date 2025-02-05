add_executable(adc_dma_capture
        dma_capture.c
        )

pico_generate_pio_header(adc_dma_capture ${CMAKE_CURRENT_LIST_DIR}/resistor_dac.pio)

target_link_libraries(adc_dma_capture
		pico_stdlib
		hardware_adc
		hardware_dma
		# For the dummy output:
		hardware_pio
		pico_multicore
		)

# create map/bin/hex file etc.
pico_add_extra_outputs(adc_dma_capture)

# add url via pico_set_program_url
example_auto_set_url(adc_dma_capture)
