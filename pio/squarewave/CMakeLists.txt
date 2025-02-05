cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    # Pull in SDK (must be before project)
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
        include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
    elseif (EXISTS ${CMAKE_CURRENT_LIST_DIR}/../../pico_sdk_import.cmake)
        # The root pico-examples one
        include(${CMAKE_CURRENT_LIST_DIR}/../../pico_sdk_import.cmake)
    else()
        message(FATAL_ERROR "Unable to locate pico_sdk_import.cmake - make sure you have copied it into this directory")
    endif()

    project(squarewave C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

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
        COMMAND pioasm -o hex ${CMAKE_CURRENT_LIST_DIR}/squarewave.pio ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave.hex
        VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave.pio.h
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/squarewave.pio
        COMMAND pioasm ${CMAKE_CURRENT_LIST_DIR}/squarewave.pio ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave.pio.h
        VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave_wrap.pio.h
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/squarewave_wrap.pio
        COMMAND pioasm ${CMAKE_CURRENT_LIST_DIR}/squarewave_wrap.pio ${CMAKE_CURRENT_LIST_DIR}/generated/squarewave_wrap.pio.h
        VERBATIM)
add_dependencies(pio_squarewave pio_squarewave_datasheet)

add_executable(pio_squarewave_div_sync)
pico_generate_pio_header(pio_squarewave_div_sync ${CMAKE_CURRENT_LIST_DIR}/squarewave.pio)
target_sources(pio_squarewave_div_sync PRIVATE
    squarewave_div_sync.c
    )
target_link_libraries(pio_squarewave_div_sync PRIVATE
    pico_stdlib hardware_pio
    )
pico_add_extra_outputs(pio_squarewave_div_sync)
example_auto_set_url(pio_squarewave_div_sync)
