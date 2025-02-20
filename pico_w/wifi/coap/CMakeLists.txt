if (DEFINED ENV{COAP_SERVER} AND NOT COAP_SERVER)
    set(COAP_SERVER $ENV{COAP_SERVER})
    message("Using COAP_SERVER from environment ('${COAP_SERVER}')")
endif()
if (NOT COAP_SERVER)
    message("Skipping COAP example as COAP_SERVER is not defined")
    return()
endif()
set(COAP_SERVER "${COAP_SERVER}" CACHE INTERNAL "COAP server for examples")
if (DEFINED ENV{LIB_COAP_DIR} AND NOT LIB_COAP_DIR)
    set(LIB_COAP_DIR $ENV{LIB_COAP_DIR})
endif()
if (NOT LIB_COAP_DIR)
    message("Skipping COAP example as LIB_COAP_DIR is not defined")
    return()
endif()
set(LIB_COAP_DIR "${LIB_COAP_DIR}" CACHE INTERNAL "LIB_COAP_DIR for examples")
if (DEFINED ENV{LIB_TINYDTLS_DIR} AND NOT LIB_TINYDTLS_DIR)
    set(LIB_TINYDTLS_DIR $ENV{LIB_TINYDTLS_DIR})
endif()
if (NOT LIB_TINYDTLS_DIR)
    message("Skipping COAP example as LIB_TINYDTLS_DIR is not defined")
    return()
endif()
set(LIB_TINYDTLS_DIR "${LIB_TINYDTLS_DIR}" CACHE INTERNAL "LIB_TINYDTLS_DIR for examples")
include(libcoap.cmake)
include(libtinydtls.cmake)

set(TARGET_NAME coap_server)
add_executable(${TARGET_NAME}
    coap_server.c
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/.. # for our common headers
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    pico_cyw43_arch_lwip_threadsafe_background
    pico_async_context_threadsafe_background
    pico_lwip_nosys
    pico_stdlib
    pico_coap
    hardware_adc
    pico_tinydtls
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    CYW43_HOST_NAME=\"pico_coap_example\"
    )
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/certs/${COAP_SERVER}")
    target_compile_definitions(${TARGET_NAME} PRIVATE
        COAP_CERT_INC=\"certs/${COAP_SERVER}/coap_server.inc\"
        )
endif()
target_compile_definitions(${TARGET_NAME} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    )
pico_add_extra_outputs(${TARGET_NAME})

set(TARGET_NAME coap_client)
add_executable(${TARGET_NAME}
    coap_client.c
    )
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
    )
target_link_libraries(${TARGET_NAME} PRIVATE
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_nosys
    pico_stdlib
    pico_coap
    pico_tinydtls
    )
target_compile_definitions(${TARGET_NAME} PRIVATE
    COAP_SERVER=\"${COAP_SERVER}\"
    )
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/certs/${COAP_SERVER}")
    target_compile_definitions(${TARGET_NAME} PRIVATE
        COAP_CERT_INC=\"certs/${COAP_SERVER}/coap_client.inc\"
        )
endif()
target_compile_definitions(${TARGET_NAME} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    )
pico_add_extra_outputs(${TARGET_NAME})
