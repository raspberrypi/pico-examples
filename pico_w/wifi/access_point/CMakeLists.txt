add_executable(picow_access_point_background
        picow_access_point.c
        dhcpserver/dhcpserver.c
        dnsserver/dnsserver.c
        )

target_include_directories(picow_access_point_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/dnsserver
        )

target_link_libraries(picow_access_point_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        )
# You can change the address below to change the address of the access point
pico_configure_ip4_address(picow_access_point_background PRIVATE
        CYW43_DEFAULT_IP_AP_ADDRESS 192.168.4.1
        )
pico_add_extra_outputs(picow_access_point_background)

add_executable(picow_access_point_poll
        picow_access_point.c
        dhcpserver/dhcpserver.c
        dnsserver/dnsserver.c
        )
target_include_directories(picow_access_point_poll PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/dnsserver
        )
target_link_libraries(picow_access_point_poll
        pico_cyw43_arch_lwip_poll
        pico_stdlib
        )
# You can change the address below to change the address of the access point
pico_configure_ip4_address(picow_access_point_poll PRIVATE
        CYW43_DEFAULT_IP_AP_ADDRESS 192.168.4.1
        )
pico_add_extra_outputs(picow_access_point_poll)
