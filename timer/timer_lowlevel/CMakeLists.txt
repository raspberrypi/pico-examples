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

    project(timer_lowlevel C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

if (PICO_ON_DEVICE)
    add_executable(timer_lowlevel
            timer_lowlevel.c)

    # Disable SDK alarm support for this lowlevel example
    set(PICO_TIME_DEFAULT_ALARM_POOL_DISABLED 1)

    target_link_libraries(timer_lowlevel pico_stdlib)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(timer_lowlevel)

    # add url via pico_set_program_url
    example_auto_set_url(timer_lowlevel)
endif ()