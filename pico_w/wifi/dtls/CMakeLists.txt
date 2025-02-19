# Pick DTLS server from environment
if (DEFINED ENV{DTLS_SERVER} AND (NOT DTLS_SERVER))
    set(DTLS_SERVER $ENV{DTLS_SERVER})
    message("Using DTLS_SERVER from environment ('${DTLS_SERVER}')")
endif()
if (NOT DTLS_SERVER)
    message("Skipping DTLS example as DTLS_SERVER is not defined")
    return()
endif()
set(DTLS_SERVER "${DTLS_SERVER}" CACHE INTERNAL "DTLS server for examples")
if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/certs/${DTLS_SERVER}")
    message("Generate DTLS certs by running ${CMAKE_CURRENT_LIST_DIR}/certs/makecerts.sh")
    return()
endif()

set(RANDOM_DATA_LEN 32)
string(RANDOM LENGTH ${RANDOM_DATA_LEN} RANDOM_DATA)

pico_add_library(pico_dtls)
target_include_directories(pico_dtls_headers INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    )
target_sources(pico_dtls INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/dtls_common.c
    )
pico_mirrored_target_link_libraries(pico_dtls INTERFACE
    pico_lwip_mbedtls
    pico_mbedtls
    )
target_compile_definitions(pico_dtls INTERFACE
    CUSTOM_MBEDTLS_ENTROPY_PTR=\"${RANDOM_DATA}\"
    CUSTOM_MBEDTLS_ENTROPY_LEN=${RANDOM_DATA_LEN}
    )

set(TARGET_NAME dtls_echo_server)

add_executable(${TARGET_NAME}
    dtls_echo_server.c
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_nosys
    pico_stdlib
    pico_dtls
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    DTLS_CERT_INC=\"certs/${DTLS_SERVER}/dtls_server.inc\"
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    CYW43_HOST_NAME=\"pico_dtls_example\"
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    )
pico_add_extra_outputs(${TARGET_NAME})

set(TARGET_NAME dtls_echo_client)

add_executable(${TARGET_NAME}
    dtls_echo_client.c
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_nosys
    pico_stdlib
    pico_dtls
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    DTLS_SERVER=\"${DTLS_SERVER}\"
    DTLS_CERT_INC=\"certs/${DTLS_SERVER}/dtls_client.inc\"
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    )
pico_add_extra_outputs(${TARGET_NAME})
