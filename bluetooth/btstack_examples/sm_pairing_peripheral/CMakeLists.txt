# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:smpairingperipheralExample
add_executable(sm_pairing_peripheral
    ../main.c
    ${PICO_BTSTACK_PATH}/example/sm_pairing_peripheral.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(sm_pairing_peripheral PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(sm_pairing_peripheral PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
# This builds a header file with details of the gatt service described in a gatt file
# note: this source for this is in pico-sdk/lib/btstack
pico_btstack_make_gatt_header(sm_pairing_peripheral PRIVATE
    ${PICO_BTSTACK_PATH}/example/sm_pairing_peripheral.gatt
)
target_compile_definitions(sm_pairing_peripheral PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(sm_pairing_peripheral)
example_auto_set_url(sm_pairing_peripheral)
