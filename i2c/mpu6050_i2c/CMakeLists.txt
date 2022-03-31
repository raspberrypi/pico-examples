add_library(MPU6050_i2c_pico_lib mpu6050_i2c.c)
target_include_directories(MPU6050_i2c_pico_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# pull in common dependencies and additional i2c hardware support
target_link_libraries(MPU6050_i2c_pico_lib pico_stdlib hardware_i2c)

add_library(MPU6050_i2c_pico_cpp_lib mpu6050.cpp mpu6050_i2c.c)
target_include_directories(MPU6050_i2c_pico_cpp_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MPU6050_i2c_pico_cpp_lib pico_stdlib hardware_i2c)


add_executable(mpu6050_i2c mpu6050_i2c_main.c)
target_link_libraries(mpu6050_i2c MPU6050_i2c_pico_lib)

# create map/bin/hex file etc.
pico_add_extra_outputs(mpu6050_i2c)
# enable usb and uart output
pico_enable_stdio_usb(mpu6050_i2c 1)
pico_enable_stdio_uart(mpu6050_i2c 1)

# add url via pico_set_program_url
example_auto_set_url(mpu6050_i2c)


add_executable(mpu6050_i2c_scale_test mpu6050_scale_test.cpp)
target_link_libraries(mpu6050_i2c_scale_test MPU6050_i2c_pico_cpp_lib)

# create map/bin/hex file etc.
pico_add_extra_outputs(mpu6050_i2c_scale_test)
pico_enable_stdio_usb(mpu6050_i2c_scale_test 1)
pico_enable_stdio_uart(mpu6050_i2c_scale_test 1)


add_executable(mpu6050_i2c_irq mpu6050_i2c_irq_main.cpp)
target_link_libraries(mpu6050_i2c_irq MPU6050_i2c_pico_cpp_lib)

# create map/bin/hex file etc.
pico_add_extra_outputs(mpu6050_i2c_irq)
pico_enable_stdio_usb(mpu6050_i2c_irq 1)
pico_enable_stdio_uart(mpu6050_i2c_irq 1)
