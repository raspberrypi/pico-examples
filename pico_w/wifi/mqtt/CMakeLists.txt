cmake_minimum_required(VERSION 3.12)

project(test_mqtt C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(mqtt_client)
pico_sdk_init()
