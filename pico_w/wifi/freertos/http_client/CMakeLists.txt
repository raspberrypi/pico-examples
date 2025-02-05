# This example is disabled as mbedtls uses too much stack, and Freertos SMP is problematic with
# pico_cyw43_arch_lwip_threadsafe_background (nosys), where WiFi and LwIP activity is performed in an IRQ.
# Prefer to use pico_cyw43_arch_lwip_sys_freertos instead, where WiFi and LwIP activity is performed in a thread.
if (0)
add_executable(picow_freertos_http_client_nosys
        picow_freertos_http_client.c
        )
target_compile_definitions(picow_freertos_http_client_nosys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        ALTCP_MBEDTLS_AUTHMODE=MBEDTLS_SSL_VERIFY_REQUIRED
        )
target_include_directories(picow_freertos_http_client_nosys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common FreeRTOSConfig
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts and mbedtls_config
        )
target_link_libraries(picow_freertos_http_client_nosys
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        example_lwip_http_util
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(picow_freertos_http_client_nosys)
endif()

add_executable(picow_freertos_http_client_sys
        picow_freertos_http_client.c
        )
target_compile_definitions(picow_freertos_http_client_sys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        NO_SYS=0 # don't want NO_SYS (generally this would be in your lwipopts.h)
        ALTCP_MBEDTLS_AUTHMODE=MBEDTLS_SSL_VERIFY_REQUIRED
        CYW43_TASK_STACK_SIZE=2048
        )
target_include_directories(picow_freertos_http_client_sys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common FreeRTOSConfig
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        )
target_link_libraries(picow_freertos_http_client_sys
        pico_cyw43_arch_lwip_sys_freertos
        pico_stdlib
        example_lwip_http_util
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
pico_add_extra_outputs(picow_freertos_http_client_sys)

# Ignore warnings from lwip code
set_source_files_properties(
        ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
        PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-result"
        )