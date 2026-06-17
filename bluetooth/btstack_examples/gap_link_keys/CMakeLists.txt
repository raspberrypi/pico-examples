# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:gaplinkkeysExample
add_executable(gap_link_keys
    ../main.c
    ${PICO_BTSTACK_PATH}/example/gap_link_keys.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(gap_link_keys PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(gap_link_keys PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(gap_link_keys PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(gap_link_keys)
example_auto_set_url(gap_link_keys)
