# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:sppflowcontrolExample
add_executable(spp_flowcontrol
    ../main.c
    ${PICO_BTSTACK_PATH}/example/spp_flowcontrol.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(spp_flowcontrol PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(spp_flowcontrol PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(spp_flowcontrol PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(spp_flowcontrol)
example_auto_set_url(spp_flowcontrol)
