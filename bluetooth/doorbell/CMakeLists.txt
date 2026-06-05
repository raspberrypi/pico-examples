# Standalone example that connects to the server Pico and flashes an external LED when the button is pressed 
# Flashes the on-board LED once quickly each second when it's running but not connected to another device
# Flashes the on-board LED twice quickly each second when connected to another device 
add_executable(ble_doorbell_client
    client.c
    )
target_link_libraries(ble_doorbell_client
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    )
target_include_directories(ble_doorbell_client PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
    )
target_compile_definitions(ble_doorbell_client PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(ble_doorbell_client)


# Standalone example that connects to the client Pico and detects button inputs
# Flashes slowly each second to show it's running
add_executable(ble_doorbell_server
    server.c 
    )
target_link_libraries(ble_doorbell_server
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    )
target_include_directories(ble_doorbell_server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
    )
target_compile_definitions(ble_doorbell_server PRIVATE
    #WANT_HCI_DUMP=1
)
pico_btstack_make_gatt_header(ble_doorbell_server PRIVATE "${CMAKE_CURRENT_LIST_DIR}/doorbell.gatt")

pico_add_extra_outputs(ble_doorbell_server)

