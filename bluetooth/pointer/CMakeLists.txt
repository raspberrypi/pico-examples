add_executable(ble_pointer 
    ble_pointer.c
    mpu6050_i2c_lib.c
)
target_link_libraries(ble_pointer
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_i2c
)
target_include_directories(ble_pointer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
)
target_compile_definitions(ble_pointer PRIVATE
    #WANT_HCI_DUMP=1
)
pico_btstack_make_gatt_header(ble_pointer PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble_pointer.gatt")
pico_add_extra_outputs(ble_pointer)

