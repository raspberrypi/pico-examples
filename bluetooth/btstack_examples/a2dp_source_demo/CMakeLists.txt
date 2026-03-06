# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:a2dpsourcedemoExample
add_executable(a2dp_source_demo
    ../main.c
    ${PICO_BTSTACK_PATH}/example/a2dp_source_demo.c # note: this source is in pico-sdk/lib/btstack
    ${PICO_BTSTACK_PATH}/src/btstack_audio_generator.c
)
target_link_libraries(a2dp_source_demo PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
    pico_btstack_hxcmod_player
    pico_btstack_sbc_encoder
)
target_include_directories(a2dp_source_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(a2dp_source_demo PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(a2dp_source_demo)
example_auto_set_url(a2dp_source_demo)
