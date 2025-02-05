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

    project(differential_manchester C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

add_executable(pio_differential_manchester)

pico_generate_pio_header(pio_differential_manchester ${CMAKE_CURRENT_LIST_DIR}/differential_manchester.pio)

target_sources(pio_differential_manchester PRIVATE differential_manchester.c)

target_link_libraries(pio_differential_manchester PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_differential_manchester)

# add url via pico_set_program_url
example_auto_set_url(pio_differential_manchester)