add_library(pio_7_segment_library INTERFACE)

target_sources(pio_7_segment_library INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/7_segment_lib.c)

# invoke pio_asm to assemble the state machine programs
#
pico_generate_pio_header(pio_7_segment_library ${CMAKE_CURRENT_LIST_DIR}/7_segment_lib.pio)

target_link_libraries(pio_7_segment_library INTERFACE
        pico_stdlib
        hardware_pio
        )

# add the `binary` directory so that the generated headers are included in the project
#
target_include_directories(pio_7_segment_library INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	)