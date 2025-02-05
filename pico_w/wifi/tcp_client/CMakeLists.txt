if (NOT TEST_TCP_SERVER_IP)
    message("Skipping tcp_client example as TEST_TCP_SERVER_IP is not defined")
else()
    add_executable(picow_tcpip_client_background
            picow_tcp_client.c
            )
    target_compile_definitions(picow_tcpip_client_background PRIVATE
            WIFI_SSID=\"${WIFI_SSID}\"
            WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
            TEST_TCP_SERVER_IP=\"${TEST_TCP_SERVER_IP}\"
            )
    target_include_directories(picow_tcpip_client_background PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
            )
    target_link_libraries(picow_tcpip_client_background
            pico_cyw43_arch_lwip_threadsafe_background
            pico_stdlib
            )

    pico_add_extra_outputs(picow_tcpip_client_background)

    add_executable(picow_tcpip_client_poll
            picow_tcp_client.c
            )
    target_compile_definitions(picow_tcpip_client_poll PRIVATE
            WIFI_SSID=\"${WIFI_SSID}\"
            WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
            TEST_TCP_SERVER_IP=\"${TEST_TCP_SERVER_IP}\"
            )
    target_include_directories(picow_tcpip_client_poll PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
            )
    target_link_libraries(picow_tcpip_client_poll
            pico_cyw43_arch_lwip_poll
            pico_stdlib
            )
    pico_add_extra_outputs(picow_tcpip_client_poll)
endif()
