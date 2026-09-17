# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:gapleadvertisementsExample
add_executable(gap_le_advertisements
    ../main.c
    ${PICO_BTSTACK_PATH}/example/gap_le_advertisements.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(gap_le_advertisements PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(gap_le_advertisements PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(gap_le_advertisements PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(gap_le_advertisements)
example_auto_set_url(gap_le_advertisements)
