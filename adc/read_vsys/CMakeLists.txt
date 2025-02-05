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

    project(read_vsys C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

add_library(power_status_adc INTERFACE)
target_sources(power_status_adc INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/power_status.c
        )
target_include_directories(power_status_adc INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_link_libraries(power_status_adc INTERFACE
        hardware_adc
        hardware_gpio
        )

add_executable(read_vsys
    read_vsys.c
    )
target_include_directories(read_vsys PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )
target_link_libraries(read_vsys
    pico_stdlib
    power_status_adc
    )
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(read_vsys
        pico_cyw43_arch_none
    )
endif()

pico_add_extra_outputs(read_vsys)
example_auto_set_url(read_vsys)
