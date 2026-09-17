# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:modplayerExample
add_executable(mod_player
    ../main.c
    ../btstack_audio_pico.c
    ${PICO_BTSTACK_PATH}/example/mod_player.c # note: this source is in pico-sdk/lib/btstack
    )
target_link_libraries(mod_player PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_hxcmod_player
    pico_audio_i2s
)
target_include_directories(mod_player PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(mod_player PRIVATE
    USING_I2C=1
    PICO_AUDIO_I2S_DATA_PIN=9
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(mod_player)
example_auto_set_url(mod_player)
