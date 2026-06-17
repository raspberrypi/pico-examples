# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:avrcpbrowsingclientExample
add_executable(avrcp_browsing_client
    ../main.c
    ${PICO_BTSTACK_PATH}/example/avrcp_browsing_client.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(avrcp_browsing_client PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(avrcp_browsing_client PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(avrcp_browsing_client PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(avrcp_browsing_client)
example_auto_set_url(avrcp_browsing_client)
