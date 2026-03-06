# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:pbapclientdemoExample
add_executable(pbap_client_demo
    ../main.c
    ${PICO_BTSTACK_PATH}/example/pbap_client_demo.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(pbap_client_demo PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(pbap_client_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(pbap_client_demo PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(pbap_client_demo)
example_auto_set_url(pbap_client_demo)
