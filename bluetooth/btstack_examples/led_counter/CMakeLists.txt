# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:ledcounterExample
add_executable(led_counter
    ../main.c
    ${PICO_BTSTACK_PATH}/example/led_counter.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(led_counter PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
)
target_include_directories(led_counter PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(led_counter PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(led_counter)
example_auto_set_url(led_counter)
