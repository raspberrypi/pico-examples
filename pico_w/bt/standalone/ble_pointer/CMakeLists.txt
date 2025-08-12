# Add mpu6050 libary
add_library(mpu6050_i2c_lib INTERFACE)
target_sources(mpu6050_i2c_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/mpu6050_i2c_lib.c
        )
target_include_directories(mpu6050_i2c_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_link_libraries(mpu6050_i2c_lib INTERFACE
    pico_stdlib
    hardware_i2c
        )

# Add executable. Default name is the project name, version 0.1
add_executable(ble_pointer 
    ble_pointer.c
    )

# Add the standard library to the build
target_link_libraries(ble_pointer
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    mpu6050_i2c_lib
    )

# Add the standard include files to the build
target_include_directories(ble_pointer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    )

pico_btstack_make_gatt_header(ble_pointer PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble_pointer.gatt")

pico_add_extra_outputs(ble_pointer)

