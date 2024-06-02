

add_executable(mqttApp
        mqtt_client.c
        )
target_link_libraries(mqttApp
        pico_stdlib
        hardware_adc
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mqtt
)

set(PICO_BOARD pico_w CACHE STRING "Board type")
# Pull in Raspberry Pi Pico SDK (must be before project)
if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_set_program_name(mqttApp "mqttApp")
pico_set_program_version(mqttApp "0.1")
pico_enable_stdio_uart(mqttApp 1)
pico_enable_stdio_usb(mqttApp 1)
target_include_directories(mqttApp PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

pico_add_extra_outputs(mqttApp)