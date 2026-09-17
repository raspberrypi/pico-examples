add_executable(ancs_client_demo
    ../main.c
    ${PICO_BTSTACK_PATH}/example/ancs_client_demo.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(ancs_client_demo PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(ancs_client_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
# This builds a header file with details of the gatt service described in a gatt file
# note: this source for this is in pico-sdk/lib/btstack
pico_btstack_make_gatt_header(ancs_client_demo PRIVATE
    ${PICO_BTSTACK_PATH}/example/ancs_client_demo.gatt
)
target_compile_definitions(ancs_client_demo PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(ancs_client_demo)
example_auto_set_url(ancs_client_demo)
