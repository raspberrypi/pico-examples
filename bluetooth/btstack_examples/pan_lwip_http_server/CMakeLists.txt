# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:panlwiphttpserverExample
add_executable(pan_lwip_http_server
    ../main.c
    ${PICO_BTSTACK_PATH}/example/pan_lwip_http_server.c # note: this source is in pico-sdk/lib/btstack
    ${PICO_BTSTACK_PATH}/3rd-party/lwip/dhcp-server/dhserver.c
)
target_link_libraries(pan_lwip_http_server PRIVATE
    pico_stdlib
    pico_cyw43_arch_threadsafe_background
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
    pico_btstack_bnep_lwip # btstack lwip
    pico_lwip_http
    pico_lwip_nosys
    )
target_include_directories(pan_lwip_http_server PRIVATE
    . # for btstack config and lwipopts.h
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config and lwipopts.h
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../../pico_w/wifi/ # for common lwip config
    ${PICO_BTSTACK_PATH}/3rd-party/lwip/dhcp-server
)
target_compile_definitions(pan_lwip_http_server PRIVATE
    CYW43_LWIP=0 # Stop cyw43 doing lwip stuff
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(pan_lwip_http_server)
example_auto_set_url(pan_lwip_http_server)
