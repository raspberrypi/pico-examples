# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:gapinquiryExample
add_executable(gap_inquiry
    ../main.c
    ${PICO_BTSTACK_PATH}/example/gap_inquiry.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(gap_inquiry PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(gap_inquiry PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(gap_inquiry PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(gap_inquiry)
example_auto_set_url(gap_inquiry)
