cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    # Pull in SDK (must be before project)
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
        include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
    elseif (EXISTS ${CMAKE_CURRENT_LIST_DIR}/../../../pico_sdk_import.cmake)
        # The root pico-examples one
        include(${CMAKE_CURRENT_LIST_DIR}/../../../pico_sdk_import.cmake)
    else()
        message(FATAL_ERROR "Unable to locate pico_sdk_import.cmake - make sure you have copied it into this directory")
    endif()

    project(blink C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

add_executable(picow_blink
        picow_blink.c
        )
target_link_libraries(picow_blink
        pico_stdlib              # for core functionality
        pico_cyw43_arch_none     # we need Wifi to access the GPIO, but we don't need anything else
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(picow_blink)

# add url via pico_set_program_url
example_auto_set_url(picow_blink)

# This version should behave exactly the same, but it runs the sys clock slower and changes the pio pio clock divisor for the cyw43 driver at run time.
add_executable(picow_blink_slow_clock
        picow_blink_slow_clock.c
        )
target_link_libraries(picow_blink_slow_clock
        pico_stdlib              # for core functionality
        pico_cyw43_arch_none     # we need Wifi to access the GPIO, but we don't need anything else
        hardware_clocks
        )
# This requires us to modify the pio divisor to successfully communicate with the cyw43 chip
target_compile_definitions(picow_blink_slow_clock PRIVATE
        CYW43_PIO_CLOCK_DIV_DYNAMIC=1
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(picow_blink_slow_clock)

# add url via pico_set_program_url
example_auto_set_url(picow_blink_slow_clock)

# This version should behave exactly the same, but it runs the sys clock faster and changes the pio pio clock divisor for the cyw43 driver at build time.
add_executable(picow_blink_fast_clock
        picow_blink_fast_clock.c
        )
target_link_libraries(picow_blink_fast_clock
        pico_stdlib              # for core functionality
        pico_cyw43_arch_none     # we need Wifi to access the GPIO, but we don't need anything else
        hardware_clocks
        )
# This requires us to modify the pio divisor to successfully communicate with the cyw43 chip
target_compile_definitions(picow_blink_fast_clock PRIVATE
        CYW43_PIO_CLOCK_DIV_INT=4
        CYW43_PIO_CLOCK_DIV_FRAC8=0
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(picow_blink_fast_clock)

# add url via pico_set_program_url
example_auto_set_url(picow_blink_fast_clock)
