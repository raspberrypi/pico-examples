# See https://bluekitchen-gmbh.com/btstack/#examples/examples/#sec:ubloxspplecounterExample
add_executable(ublox_spp_le_counter
    ../main.c
    ${PICO_BTSTACK_PATH}/example/ublox_spp_le_counter.c # note: this source is in pico-sdk/lib/btstack
)
target_link_libraries(ublox_spp_le_counter PRIVATE
    pico_stdlib
    pico_cyw43_arch_none # we don't need lwip support
    pico_btstack_cyw43 # enable bt
    pico_btstack_ble # ble support
    pico_btstack_classic # classic support
)
target_include_directories(ublox_spp_le_counter PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # for btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../config # for common config
)
# This builds a header file with details of the gatt service described in a gatt file
# note: this source for this is in pico-sdk/lib/btstack
pico_btstack_make_gatt_header(ublox_spp_le_counter PRIVATE
    ${PICO_BTSTACK_PATH}/example/ublox_spp_le_counter.gatt
)
target_compile_definitions(ublox_spp_le_counter PRIVATE
    #WANT_HCI_DUMP=1
)
pico_add_extra_outputs(ublox_spp_le_counter)
example_auto_set_url(ublox_spp_le_counter)
