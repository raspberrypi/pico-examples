set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR})

add_executable(picow_httpd_background
        pico_httpd.c
        )
target_compile_definitions(picow_httpd_background PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(picow_httpd_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
        )
target_link_libraries(picow_httpd_background PRIVATE
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_http
        pico_lwip_mdns
        pico_httpd_content
        pico_stdlib
        )
pico_add_extra_outputs(picow_httpd_background)

pico_add_library(pico_httpd_content NOFLAG)
pico_set_lwip_httpd_content(pico_httpd_content INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/content/404.html
        ${CMAKE_CURRENT_LIST_DIR}/content/index.shtml
        ${CMAKE_CURRENT_LIST_DIR}/content/test.shtml
        ${CMAKE_CURRENT_LIST_DIR}/content/ledpass.shtml
        ${CMAKE_CURRENT_LIST_DIR}/content/ledfail.shtml
        ${CMAKE_CURRENT_LIST_DIR}/content/img/rpi.png
        )
