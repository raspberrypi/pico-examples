# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:sppcounterExample
add_executable(spp_counter
    ../main.c
    ${PICO_BTSTACK_PATH}/example/spp_counter.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(spp_counter PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(spp_counter PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
target_compile_definitions(spp_counter PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(spp_counter)
example_auto_set_url(spp_counter)
