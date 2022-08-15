add_library(picow_iperf_common INTERFACE)
target_sources(picow_iperf_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/picow_iperf.c
        )
target_compile_definitions(picow_iperf_common INTERFACE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_iperf_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_iperf_common INTERFACE
        pico_stdlib
        pico_lwip_iperf
        )

add_executable(picow_iperf_server_background)
target_link_libraries(picow_iperf_server_background
        pico_cyw43_arch_lwip_threadsafe_background
        picow_iperf_common
        )
pico_add_extra_outputs(picow_iperf_server_background)

add_executable(picow_iperf_server_poll)
target_link_libraries(picow_iperf_server_poll
        pico_cyw43_arch_lwip_poll
        picow_iperf_common
        )
pico_add_extra_outputs(picow_iperf_server_poll)

# Add iperf client examples
if(NOT IPERF_SERVER_IP)
        message("Skipping iperf client examples as IPERF_SERVER_IP is not defined")
else()
        add_executable(picow_iperf_client_background)
        target_compile_definitions(picow_iperf_client_background PRIVATE
                IPERF_SERVER_IP=\"${IPERF_SERVER_IP}\"
                IPERF_CLIENT=1
                )
        target_link_libraries(picow_iperf_client_background
                pico_cyw43_arch_lwip_threadsafe_background
                picow_iperf_common
                )
        pico_add_extra_outputs(picow_iperf_client_background)

        add_executable(picow_iperf_client_poll)
        target_compile_definitions(picow_iperf_client_poll PRIVATE
                IPERF_SERVER_IP=\"${IPERF_SERVER_IP}\"
                IPERF_CLIENT=1
                )
        target_link_libraries(picow_iperf_client_poll
                pico_cyw43_arch_lwip_poll
                picow_iperf_common
                )
        pico_add_extra_outputs(picow_iperf_client_poll)
endif()