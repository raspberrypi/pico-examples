add_executable(picow_ntp_client_background
        picow_ntp_client.c
        )
target_compile_definitions(picow_ntp_client_background PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_ntp_client_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
)
target_link_libraries(picow_ntp_client_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        )

pico_add_extra_outputs(picow_ntp_client_background)

add_executable(picow_ntp_client_poll
        picow_ntp_client.c
        )
target_compile_definitions(picow_ntp_client_poll PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_ntp_client_poll PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_ntp_client_poll
        pico_cyw43_arch_lwip_poll
        pico_stdlib
        )
pico_add_extra_outputs(picow_ntp_client_poll)

