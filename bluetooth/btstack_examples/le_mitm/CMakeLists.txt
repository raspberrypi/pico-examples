# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:lemitmExample
add_executable(le_mitm
    ../main.c
    ${PICO_BTSTACK_PATH}/example/le_mitm.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(le_mitm PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(le_mitm PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(le_mitm PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(le_mitm)
example_auto_set_url(le_mitm)
