add_executable(picow_wifi_scan_background
        picow_wifi_scan.c
        )
target_include_directories(picow_wifi_scan_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_wifi_scan_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        )

pico_add_extra_outputs(picow_wifi_scan_background)

add_executable(picow_wifi_scan_poll
        picow_wifi_scan.c
        )
target_include_directories(picow_wifi_scan_poll PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_wifi_scan_poll
        pico_cyw43_arch_lwip_poll
        pico_stdlib
        )
pico_add_extra_outputs(picow_wifi_scan_poll)

