add_executable(picow_freertos_iperf_server_nosys
        picow_freertos_iperf.c
        )
target_compile_definitions(picow_freertos_iperf_server_nosys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_freertos_iperf_server_nosys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        )
target_link_libraries(picow_freertos_iperf_server_nosys
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_lwip_iperf
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(picow_freertos_iperf_server_nosys)

add_executable(picow_freertos_iperf_server_sys
        picow_freertos_iperf.c
        )
target_compile_definitions(picow_freertos_iperf_server_sys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        )
target_include_directories(picow_freertos_iperf_server_sys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        )
target_link_libraries(picow_freertos_iperf_server_sys
        pico_cyw43_arch_lwip_sys_freertos
        pico_stdlib
        pico_lwip_iperf
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(picow_freertos_iperf_server_sys)
