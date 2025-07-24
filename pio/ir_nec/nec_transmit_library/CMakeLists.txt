add_library(nec_transmit_library INTERFACE)

target_sources(nec_transmit_library INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/nec_transmit.c)

# invoke pio_asm to assemble the PIO state machine programs
#
pico_generate_pio_header(nec_transmit_library ${CMAKE_CURRENT_LIST_DIR}/nec_carrier_burst.pio)
pico_generate_pio_header(nec_transmit_library ${CMAKE_CURRENT_LIST_DIR}/nec_carrier_control.pio)

target_link_libraries(nec_transmit_library INTERFACE
        pico_stdlib
        hardware_pio
        )

# add the `binary` directory so that the generated headers are included in the project
#
target_include_directories (nec_transmit_library INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	)
