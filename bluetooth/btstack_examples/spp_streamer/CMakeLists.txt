# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:sppstreamerExample
add_executable(spp_streamer
    ../main.c
    ${PICO_BTSTACK_PATH}/example/spp_streamer.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(spp_streamer PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(spp_streamer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(spp_streamer PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(spp_streamer)
example_auto_set_url(spp_streamer)
