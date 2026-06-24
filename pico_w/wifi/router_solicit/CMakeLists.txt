add_executable(picow_router_solicit
    router_solicit.c
)
target_link_libraries(picow_router_solicit
    pico_stdlib
    pico_cyw43_arch_none
)
target_compile_definitions(picow_router_solicit PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)
pico_add_extra_outputs(picow_router_solicit)
