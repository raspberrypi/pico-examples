# Standalone example that connects to picow_ble_temp_sensor and reads the temperature
# Flahes once quickly each second when it's running but not connected to another device
# Flashes twice quickly each second when connected to another device and reading it's temperature
add_executable(picow_ble_temp_reader
    client.c
    )
target_link_libraries(picow_ble_temp_reader
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    )
target_include_directories(picow_ble_temp_reader PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/.. # For our common btstack config
    )
target_compile_definitions(picow_ble_temp_reader PRIVATE
    RUNNING_AS_CLIENT=1
)

pico_add_extra_outputs(picow_ble_temp_reader)
example_auto_set_url(picow_ble_temp_reader)
