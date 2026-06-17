# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:gattstreamerserverExample
add_executable(gatt_streamer_server
    ../main.c
    ${PICO_BTSTACK_PATH}/example/gatt_streamer_server.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(gatt_streamer_server PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(gatt_streamer_server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
# This builds a header file with details of the gatt service described in a gatt file
# note: this source for this is in pico-sdk/lib/btstack
pico_btstack_make_gatt_header(gatt_streamer_server PRIVATE
    ${PICO_BTSTACK_PATH}/example/gatt_streamer_server.gatt
)
target_compile_definitions(gatt_streamer_server PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(gatt_streamer_server)
example_auto_set_url(gatt_streamer_server)
