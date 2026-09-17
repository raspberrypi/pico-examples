# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:nordicspplestreamerExample
add_executable(nordic_spp_le_streamer
    ../main.c
    ${PICO_BTSTACK_PATH}/example/nordic_spp_le_streamer.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(nordic_spp_le_streamer PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(nordic_spp_le_streamer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
# This builds a header file with details of the gatt service described in a gatt file
# note: this source for this is in pico-sdk/lib/btstack
pico_btstack_make_gatt_header(nordic_spp_le_streamer PRIVATE
    ${PICO_BTSTACK_PATH}/example/nordic_spp_le_streamer.gatt
)
target_compile_definitions(nordic_spp_le_streamer PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(nordic_spp_le_streamer)
example_auto_set_url(nordic_spp_le_streamer)
