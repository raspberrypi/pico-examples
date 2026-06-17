# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:sppandgattcounterExample
add_executable(spp_and_gatt_counter
    ../main.c
    ${PICO_BTSTACK_PATH}/example/spp_and_gatt_counter.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(spp_and_gatt_counter PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(spp_and_gatt_counter PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
# This builds a header file with details of the gatt service described in a gatt file
# note: this source for this is in pico-sdk/lib/btstack
pico_btstack_make_gatt_header(spp_and_gatt_counter PRIVATE
    ${PICO_BTSTACK_PATH}/example/spp_and_gatt_counter.gatt
)
target_compile_definitions(spp_and_gatt_counter PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(spp_and_gatt_counter)
example_auto_set_url(spp_and_gatt_counter)
