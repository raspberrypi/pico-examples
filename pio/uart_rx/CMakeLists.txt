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

    project(uart_rx C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

add_executable(pio_uart_rx)

pico_generate_pio_header(pio_uart_rx ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)

target_sources(pio_uart_rx PRIVATE uart_rx.c)

target_link_libraries(pio_uart_rx PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        )

pico_add_extra_outputs(pio_uart_rx)

# add url via pico_set_program_url
example_auto_set_url(pio_uart_rx)

# Similar to above but uses an interrupt for RX
add_executable(pio_uart_rx_intr)
pico_generate_pio_header(pio_uart_rx_intr ${CMAKE_CURRENT_LIST_DIR}/uart_rx.pio)
target_sources(pio_uart_rx_intr PRIVATE uart_rx_intr.c)
target_link_libraries(pio_uart_rx_intr PRIVATE
        pico_stdlib
        pico_multicore
        hardware_pio
        pico_async_context_threadsafe_background
        )
pico_add_extra_outputs(pio_uart_rx_intr)
example_auto_set_url(pio_uart_rx_intr)
