add_executable(ble_pointer 
    ble_pointer.c
    ble_pointer_nosys_main.c
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

if (NOT FREERTOS_KERNEL_PATH AND NOT DEFINED ENV{FREERTOS_KERNEL_PATH})
    return()
endif()
include(../../freertos/FreeRTOS_Kernel_import.cmake)

add_executable(ble_pointer_freertos
    ble_pointer.c
    ble_pointer_freertos_main.c
    mpu6050_i2c_lib.c
)
target_link_libraries(ble_pointer_freertos
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_sys_freertos
    hardware_i2c
    FreeRTOS-Kernel-Heap4
)
target_include_directories(ble_pointer_freertos PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../config # for common btstack config
    ${CMAKE_CURRENT_LIST_DIR}/../../freertos # for FreeRTOSConfig.h
)
target_compile_definitions(ble_pointer_freertos PRIVATE
    #WANT_HCI_DUMP=1
    CYW43_LWIP=0
    configNUMBER_OF_CORES=1 # run freertos on one core, the other core is doing something else
)
pico_btstack_make_gatt_header(ble_pointer_freertos PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble_pointer.gatt")
pico_add_extra_outputs(ble_pointer_freertos)

