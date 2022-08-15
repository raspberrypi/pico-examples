add_library(picow_freertos_iperf_common INTERFACE)
target_sources(picow_freertos_iperf_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/picow_freertos_iperf.c
        )
target_compile_definitions(picow_freertos_iperf_common INTERFACE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_freertos_iperf_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        )
target_link_libraries(picow_freertos_iperf_common INTERFACE
        pico_stdlib
        pico_lwip_iperf
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )

add_executable(picow_freertos_iperf_server_nosys)
target_link_libraries(picow_freertos_iperf_server_nosys
        pico_cyw43_arch_lwip_threadsafe_background
        picow_freertos_iperf_common
        )
pico_add_extra_outputs(picow_freertos_iperf_server_nosys)

add_executable(picow_freertos_iperf_server_sys)
target_compile_definitions(picow_freertos_iperf_server_sys PRIVATE
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        )
target_link_libraries(picow_freertos_iperf_server_sys
        pico_cyw43_arch_lwip_sys_freertos
        picow_freertos_iperf_common
        )
pico_add_extra_outputs(picow_freertos_iperf_server_sys)

if(IPERF_SERVER_IP)
        add_executable(picow_freertos_iperf_client_nosys)
        target_compile_definitions(picow_freertos_iperf_client_nosys PRIVATE
                IPERF_SERVER_IP=\"${IPERF_SERVER_IP}\"
                IPERF_CLIENT=1
                )
        target_link_libraries(picow_freertos_iperf_client_nosys
                pico_cyw43_arch_lwip_threadsafe_background
                picow_freertos_iperf_common
                )
        pico_add_extra_outputs(picow_freertos_iperf_client_nosys)

        add_executable(picow_freertos_iperf_client_sys)
        target_compile_definitions(picow_freertos_iperf_client_sys PRIVATE
                NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
                IPERF_SERVER_IP=\"${IPERF_SERVER_IP}\"
                IPERF_CLIENT=1
                )
        target_link_libraries(picow_freertos_iperf_client_sys
                pico_cyw43_arch_lwip_sys_freertos
                picow_freertos_iperf_common
                )
        pico_add_extra_outputs(picow_freertos_iperf_client_sys)
endif()
