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

    project(udp_beacon C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

add_executable(picow_udp_beacon_background
        picow_udp_beacon.c
        )
target_compile_definitions(picow_udp_beacon_background PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_udp_beacon_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_udp_beacon_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        )

pico_add_extra_outputs(picow_udp_beacon_background)

add_executable(picow_udp_beacon_poll
        picow_udp_beacon.c
        )
target_compile_definitions(picow_udp_beacon_poll PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_udp_beacon_poll PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_udp_beacon_poll
        pico_cyw43_arch_lwip_poll
        pico_stdlib
        )
pico_add_extra_outputs(picow_udp_beacon_poll)
