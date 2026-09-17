# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:sppstreamerExample
add_executable(spp_streamer_with_wifi
    ../main.c
    ${PICO_BTSTACK_PATH}/example/spp_streamer.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(spp_streamer_with_wifi PRIVATE
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background # we want lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
    pico_lwip_iperf
)
target_include_directories(spp_streamer_with_wifi PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack and lwip config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../../pico_w/wifi # for common lwip config
)
target_compile_definitions(spp_streamer_with_wifi PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    USING_IPERF=1
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(spp_streamer_with_wifi)
example_auto_set_url(spp_streamer_with_wifi)
