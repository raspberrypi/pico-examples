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

    project(hello_anything C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

if (TARGET tinyusb_device)
    add_executable(hello_anything
            hello_anything.c
            )

    # pull in common dependencies
    target_link_libraries(hello_anything pico_stdlib)

    # enable usb and uart output
    pico_enable_stdio_usb(hello_anything 1)
    pico_enable_stdio_uart(hello_anything 1)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(hello_anything)

    # add url via pico_set_program_url
    example_auto_set_url(hello_anything)
elseif(PICO_ON_DEVICE)
    message("Skipping hello_anything because TinyUSB submodule is not initialized in the SDK")
endif()
