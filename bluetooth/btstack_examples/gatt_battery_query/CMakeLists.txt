# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:gattbatteryqueryExample
add_executable(gatt_battery_query
    ../main.c
    ${PICO_BTSTACK_PATH}/example/gatt_battery_query.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(gatt_battery_query PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(gatt_battery_query PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
# This builds a header file with details of the gatt service described in a gatt file
# note: this source for this is in pico-sdk/lib/btstack
pico_btstack_make_gatt_header(gatt_battery_query PRIVATE
    ${PICO_BTSTACK_PATH}/example/gatt_battery_query.gatt
)
target_compile_definitions(gatt_battery_query PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(gatt_battery_query)
example_auto_set_url(gatt_battery_query)
