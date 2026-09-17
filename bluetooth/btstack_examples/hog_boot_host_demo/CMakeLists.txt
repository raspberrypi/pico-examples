# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:hogboothostdemoExample
add_executable(hog_boot_host_demo
    ../main.c
    ${PICO_BTSTACK_PATH}/example/hog_boot_host_demo.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(hog_boot_host_demo PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(hog_boot_host_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(hog_boot_host_demo PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(hog_boot_host_demo)
example_auto_set_url(hog_boot_host_demo)
