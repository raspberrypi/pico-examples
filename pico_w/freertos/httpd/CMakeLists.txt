set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR})
find_package(makefsdata REQUIRED)

set(PICO_HTTP_CONTENT_IN_DIR ${CMAKE_CURRENT_LIST_DIR}/content)
set(PICO_HTTP_CONTENT_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/content)

add_executable(picow_freertos_httpd_nosys
        picow_freertos_httpd.c
        )
target_compile_definitions(picow_freertos_httpd_nosys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_freertos_httpd_nosys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        ${PICO_HTTP_CONTENT_OUT_DIR} # For the http content
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
        )
target_link_libraries(picow_freertos_httpd_nosys
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_http
        pico_lwip_mdns
        pico_stdlib
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
add_dependencies(picow_freertos_httpd_nosys picow_freertos_httpd_content)
pico_add_extra_outputs(picow_freertos_httpd_nosys)

add_executable(picow_freertos_httpd_sys
        picow_freertos_httpd.c
        )
target_compile_definitions(picow_freertos_httpd_sys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        LWIP_SOCKET=1       # we need the socket API (generally this would be in your lwipopts.h)
        )
target_include_directories(picow_freertos_httpd_sys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        ${PICO_HTTP_CONTENT_OUT_DIR} # For the http content
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
        )
target_link_libraries(picow_freertos_httpd_sys
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_http
        pico_lwip_mdns
        pico_stdlib
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )
add_dependencies(picow_freertos_httpd_sys picow_freertos_httpd_content)
pico_add_extra_outputs(picow_freertos_httpd_sys)

# Build the content
add_custom_target(picow_freertos_httpd_content DEPENDS
        ${PICO_HTTP_CONTENT_OUT_DIR}/picow_fsdata.c
        )
add_custom_command(OUTPUT ${PICO_HTTP_CONTENT_OUT_DIR}/picow_fsdata.c
        DEPENDS makefsdata ${PICO_HTTP_CONTENT_IN_DIR}/*.html ${PICO_HTTP_CONTENT_IN_DIR}/*.shtml
        COMMAND mkdir -p ${PICO_HTTP_CONTENT_OUT_DIR} &&
                ${CMAKE_CURRENT_BINARY_DIR}/makefsdata/makefsdata ${PICO_HTTP_CONTENT_IN_DIR} -f:${PICO_HTTP_CONTENT_OUT_DIR}/picow_fsdata.c
        )
