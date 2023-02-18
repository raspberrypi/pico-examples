add_library(onewire_library
        onewire_library.c
        )

# invoke pio_asm to assemble the state machine programs
#
pico_generate_pio_header(onewire_library ${CMAKE_CURRENT_LIST_DIR}/onewire_library.pio)

target_link_libraries(onewire_library PRIVATE
        pico_stdlib
        hardware_pio
        )

# add the `binary` directory so that the generated headers are included in the project
#
target_include_directories(onewire_library PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	)