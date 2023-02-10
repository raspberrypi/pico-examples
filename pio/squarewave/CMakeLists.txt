add_executable(pio_squarewave)

pico_generate_pio_header(pio_squarewave ${CMAKE_CURRENT_LIST_DIR}/squarewave.pio)
pico_generate_pio_header(pio_squarewave ${CMAKE_CURRENT_LIST_DIR}/squarewave_wrap.pio)
pico_generate_pio_header(pio_squarewave ${CMAKE_CURRENT_LIST_DIR}/squarewave_fast.pio)

target_sources(pio_squarewave PRIVATE squarewave.c)

target_link_libraries(pio_squarewave PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_squarewave)

# add url via pico_set_program_url
example_auto_set_url(pio_squarewave)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
# generate .hex file and .pio.h file for the RP2040 datasheet (to make sure
# the datasheet always shows the output of the latest pioasm version)
add_custom_target(pio_squarewave_datasheet DEPENDS
		${CMAKE_CURRENT_LIST_DIR}/generated/squarewave.hex
		${CMAKE_CURRENT_LIST_DIR}/generated/squarewave.pio.h
		${CMAKE_CURRENT_LIST_DIR}/generated/squarewave_wrap.pio.h
		)
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave.hex
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/squarewave.pio
        COMMAND Pioasm -o hex ${CMAKE_CURRENT_LIST_DIR}/squarewave.pio ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave.hex
        VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave.pio.h
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/squarewave.pio
        COMMAND Pioasm ${CMAKE_CURRENT_LIST_DIR}/squarewave.pio ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave.pio.h
        VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave_wrap.pio.h
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/squarewave_wrap.pio
        COMMAND Pioasm ${CMAKE_CURRENT_LIST_DIR}/squarewave_wrap.pio ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave_wrap.pio.h
        VERBATIM)
add_dependencies(pio_squarewave pio_squarewave_datasheet)
