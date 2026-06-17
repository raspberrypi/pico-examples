# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:attdelayedresponseExample
add_executable(att_delayed_response
    ../main.c
    ${PICO_BTSTACK_PATH}/example/att_delayed_response.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(att_delayed_response PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(att_delayed_response PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
# This builds a header file with details of the gatt service described in a gatt file
# note: this source for this is in pico-sdk/lib/btstack
pico_btstack_make_gatt_header(att_delayed_response PRIVATE
    ${PICO_BTSTACK_PATH}/example/att_delayed_response.gatt
)
target_compile_definitions(att_delayed_response PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(att_delayed_response)
example_auto_set_url(att_delayed_response)
