# Define the host name of the MQTT server in an environment variable or pass it to cmake,
# e.g. cmake -DMQTT_SERVER=myserver ..

if (DEFINED ENV{MQTT_SERVER} AND (NOT MQTT_SERVER))
    set(MQTT_SERVER $ENV{MQTT_SERVER})
    message("Using MQTT_SERVER from environment ('${MQTT_SERVER}')")
endif()
if (NOT MQTT_SERVER)
    return()
endif()
if (DEFINED ENV{MQTT_USERNAME} AND (NOT MQTT_USERNAME))
    set(MQTT_USERNAME $ENV{MQTT_USERNAME})
    message("Using MQTT_USERNAME from environment ('${MQTT_USERNAME}')")
endif()
if (DEFINED ENV{MQTT_PASSWORD} AND (NOT MQTT_PASSWORD))
    set(MQTT_PASSWORD $ENV{MQTT_PASSWORD})
    message("Using MQTT_PASSWORD from environment")
endif()

# Define the name of an MQTT broker/server to enable this example
set(MQTT_SERVER "${MQTT_SERVER}" CACHE INTERNAL "MQTT server for examples")

set(TARGET_NAME mqtt_client)

add_executable(${TARGET_NAME}
    mqtt_client.c
)
target_link_libraries(${TARGET_NAME}
    pico_stdlib
    hardware_adc
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mqtt
)
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts or any other standard includes, if required
)
target_compile_definitions(${TARGET_NAME} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    MQTT_SERVER=\"${MQTT_SERVER}\"
)
if (MQTT_USERNAME AND MQTT_PASSWORD)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        MQTT_USERNAME=\"${MQTT_USERNAME}\"
        MQTT_PASSWORD=\"${MQTT_PASSWORD}\"
    )
endif()
pico_add_extra_outputs(${TARGET_NAME})
