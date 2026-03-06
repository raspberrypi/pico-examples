# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:hogkeyboarddemoExample
add_executable(hog_keyboard_demo
    ../main.c
    ${PICO_BTSTACK_PATH}/example/hog_keyboard_demo.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(hog_keyboard_demo PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(hog_keyboard_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
# This builds a header file with details of the gatt service described in a gatt file
# note: this source for this is in pico-sdk/lib/btstack
pico_btstack_make_gatt_header(hog_keyboard_demo PRIVATE
    ${PICO_BTSTACK_PATH}/example/hog_keyboard_demo.gatt
)
target_compile_definitions(hog_keyboard_demo PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(hog_keyboard_demo)
example_auto_set_url(hog_keyboard_demo)
