add_executable(picow_freertos_ntp_client_socket
        picow_freertos_ntp_client_socket.c
        )
target_compile_definitions(picow_freertos_ntp_client_socket PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        LWIP_SOCKET=1       # we need the socket API (generally this would be in your lwipopts.h)
        )
target_include_directories(picow_freertos_ntp_client_socket PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common FreeRTOSConfig
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        )
target_link_libraries(picow_freertos_ntp_client_socket
        pico_cyw43_arch_lwip_sys_freertos
        pico_stdlib
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(picow_freertos_ntp_client_socket)
