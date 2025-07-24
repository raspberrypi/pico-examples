# Threadsafe

add_executable(picow_udp_server_background
        picow_udp_server.c
        )
target_compile_definitions(picow_udp_server_background PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_udp_server_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for the common lwipopts.h
        )
target_link_libraries(picow_udp_server_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        )

pico_add_extra_outputs(picow_udp_server_background)

pico_enable_stdio_usb(picow_udp_server_background 1)

# Manual polling

add_executable(picow_udp_server_poll
        picow_udp_server.c
        )
target_compile_definitions(picow_udp_server_poll PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_udp_server_poll PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_udp_server_poll
        pico_cyw43_arch_lwip_poll
        pico_stdlib
        )
        
pico_add_extra_outputs(picow_udp_server_poll)

pico_enable_stdio_usb(picow_udp_server_poll 1)
