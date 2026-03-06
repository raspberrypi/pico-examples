# Add provisioning libary
add_library(wifi_prov_lib INTERFACE)
target_sources(wifi_prov_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/wifi_prov_lib.c
)
target_include_directories(wifi_prov_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
    ${CMAKE_CURRENT_LIST_DIR}../../pico_w/wifi # for common lwip config
)
target_link_libraries(wifi_prov_lib INTERFACE
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    hardware_flash
)
pico_btstack_make_gatt_header(wifi_prov_lib INTERFACE "${CMAKE_CURRENT_LIST_DIR}/provisioning.gatt")

# Example which uses the provisioning libary to allow credentials to be set over BLE
add_executable(wifi_provisioner
    example.c 
)
target_link_libraries(wifi_provisioner
    pico_stdlib
    wifi_prov_lib
    pico_lwip_iperf
    pico_cyw43_arch_lwip_threadsafe_background
)
target_include_directories(wifi_provisioner PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
)
pico_add_extra_outputs(wifi_provisioner)
