# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:sdpgeneralqueryExample
add_executable(sdp_general_query
    ../main.c
    ${PICO_BTSTACK_PATH}/example/sdp_general_query.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(sdp_general_query PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(sdp_general_query PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(sdp_general_query PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(sdp_general_query)
example_auto_set_url(sdp_general_query)
