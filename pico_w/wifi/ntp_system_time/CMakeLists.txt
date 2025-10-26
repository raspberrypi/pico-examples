add_executable(picow_ntp_system_time
        ntp_system_time.c
        )
target_compile_definitions(picow_ntp_system_time PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_ntp_system_time PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
)
target_link_libraries(picow_ntp_system_time
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_sntp  # LWIP sntp application
        pico_aon_timer  # high-level API for "always on" timer
        pico_sync       # thread synchronisation (mutex)
        pico_stdlib
        )

pico_add_extra_outputs(picow_ntp_system_time)