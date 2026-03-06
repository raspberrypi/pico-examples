add_executable(gap_dedicated_bonding
    ../main.c
    ${PICO_BTSTACK_PATH}/example/gap_dedicated_bonding.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(gap_dedicated_bonding PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(gap_dedicated_bonding PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(gap_dedicated_bonding PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(gap_dedicated_bonding)
example_auto_set_url(gap_dedicated_bonding)
