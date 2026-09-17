# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:dutmodeclassicExample
add_executable(dut_mode_classic
    ../main.c
    ${PICO_BTSTACK_PATH}/example/dut_mode_classic.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(dut_mode_classic PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(dut_mode_classic PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(dut_mode_classic PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(dut_mode_classic)
example_auto_set_url(dut_mode_classic)
