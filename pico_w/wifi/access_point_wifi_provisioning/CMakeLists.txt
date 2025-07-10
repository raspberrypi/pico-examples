add_executable(AP_provisioning
        AP_provisioning.c
        dhcpserver/dhcpserver.c
        dnsserver/dnsserver.c
        )
target_include_directories(AP_provisioning PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
        ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/dnsserver
        )
target_link_libraries(AP_provisioning PRIVATE
        pico_cyw43_arch_lwip_poll
        pico_lwip_http
        ap_provisioning_content
        pico_stdlib
        hardware_flash
        )
pico_add_extra_outputs(AP_provisioning)

pico_add_library(ap_provisioning_content NOFLAG)
pico_set_lwip_httpd_content(ap_provisioning_content INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/content/index.shtml
        )