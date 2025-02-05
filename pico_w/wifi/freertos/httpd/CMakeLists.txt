set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR})

add_executable(pico_freertos_httpd_nosys
        pico_freertos_httpd.c
        )
target_compile_definitions(pico_freertos_httpd_nosys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(pico_freertos_httpd_nosys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common FreeRTOSConfig
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
        )
target_link_libraries(pico_freertos_httpd_nosys
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_http
        pico_lwip_mdns
        pico_stdlib
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        pico_freertos_httpd_content
        )
pico_add_extra_outputs(pico_freertos_httpd_nosys)

add_executable(pico_freertos_httpd_sys
        pico_freertos_httpd.c
        )
target_compile_definitions(pico_freertos_httpd_sys PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        LWIP_SOCKET=1       # we need the socket API (generally this would be in your lwipopts.h)
        )
target_include_directories(pico_freertos_httpd_sys PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common FreeRTOSConfig
        ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
        )
target_link_libraries(pico_freertos_httpd_sys
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_http
        pico_lwip_mdns
        pico_stdlib
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        pico_freertos_httpd_content
        )
pico_add_extra_outputs(pico_freertos_httpd_sys)

pico_add_library(pico_freertos_httpd_content NOFLAG)
pico_set_lwip_httpd_content(pico_freertos_httpd_content INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/content/404.html
        ${CMAKE_CURRENT_LIST_DIR}/content/index.shtml
        ${CMAKE_CURRENT_LIST_DIR}/content/test.shtml
        )
