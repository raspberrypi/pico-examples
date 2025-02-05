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

    project(onboard_temperature C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

add_executable(onboard_temperature onboard_temperature.c)

target_link_libraries(onboard_temperature pico_stdlib hardware_adc)

# enable uart output, disable usb output
pico_enable_stdio_uart(onboard_temperature 1)
pico_enable_stdio_usb(onboard_temperature 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(onboard_temperature)

# add url via pico_set_program_url
example_auto_set_url(onboard_temperature)

