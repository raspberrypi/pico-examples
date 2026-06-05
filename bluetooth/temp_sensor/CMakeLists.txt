# Example that reads from the on board temperature sensor and sends notifications via BLE
# Flashes slowly each second to show it's running
add_executable(ble_temp_server
    server.c
    )
target_link_libraries(ble_temp_server
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    )
target_include_directories(ble_temp_server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For our btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
    )
target_compile_definitions(ble_temp_server PRIVATE
    #WANT_HCI_DUMP=1
)
pico_btstack_make_gatt_header(ble_temp_server PRIVATE "${CMAKE_CURRENT_LIST_DIR}/temp_sensor.gatt")

pico_add_extra_outputs(ble_temp_server)
example_auto_set_url(ble_temp_server)

# Example that connects to ble_temp_server and reads the temperature
# Flashes once quickly each second when it's running but not connected to another device
# Flashes twice quickly each second when connected to another device and reading it's temperature
add_executable(ble_temp_reader
    client.c
    )
target_link_libraries(ble_temp_reader
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    )
target_include_directories(ble_temp_reader PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For our btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
    )
target_compile_definitions(ble_temp_reader PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(ble_temp_reader)
example_auto_set_url(ble_temp_reader)
