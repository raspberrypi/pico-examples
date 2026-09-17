# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:gattheartrateclientExample
add_executable(gatt_heart_rate_client
    ../main.c
    ${PICO_BTSTACK_PATH}/example/gatt_heart_rate_client.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(gatt_heart_rate_client PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(gatt_heart_rate_client PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(gatt_heart_rate_client PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(gatt_heart_rate_client)
example_auto_set_url(gatt_heart_rate_client)
