# Select a security setting to explore the BLE security. See README.md for details
if (NOT DEFINED SECURITY_SETTING)
    set(SECURITY_SETTING 0)
endif()

# Standalone example that reads from the on board temperature sensor and sends notifications via BLE
# Flashes slowly each second to show it's running
add_executable(ble_secure_temp_server
    server.c 
)
target_link_libraries(ble_secure_temp_server
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
)
target_include_directories(ble_secure_temp_server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For our btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
)
target_compile_definitions(ble_secure_temp_server PRIVATE
    # Wait up to 3s for stdio USB to initialise, to avoid losing any debug on startup
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
    SECURITY_SETTING=${SECURITY_SETTING}
    #WANT_HCI_DUMP=1
)
pico_btstack_make_gatt_header(ble_secure_temp_server PRIVATE "${CMAKE_CURRENT_LIST_DIR}/temp_sensor.gatt")
pico_add_extra_outputs(ble_secure_temp_server)
pico_enable_stdio_usb(ble_secure_temp_server 1)
pico_enable_stdio_uart(ble_secure_temp_server 1)

# Standalone example that connects to ble_secure_temp_server and reads the temperature 
# Flahes once quickly each second when it's running but not connected to another device
# Flashes twice quickly each second when connected to another device and reading it's temperature
add_executable(ble_secure_temp_client
    client.c
    )
target_link_libraries(ble_secure_temp_client
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    )
target_include_directories(ble_secure_temp_client PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For our btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
    )
target_compile_definitions(ble_secure_temp_client PRIVATE
    # Wait up to 3s for stdio USB to initialise, to avoid losing any debug on startup
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
    SECURITY_SETTING=${SECURITY_SETTING}
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(ble_secure_temp_client)
pico_enable_stdio_usb(ble_secure_temp_client 1)
pico_enable_stdio_uart(ble_secure_temp_client 1)
