add_executable(picow_ota_update_background
        picow_ota_update.c
        )
target_compile_definitions(picow_ota_update_background PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        PICO_CRT0_IMAGE_TYPE_TBYB=1
        )
target_include_directories(picow_ota_update_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_ota_update_background
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_sha256
        boot_uf2_headers
        )

pico_use_wifi_firmware_partition(picow_ota_update_background)

pico_hash_binary(picow_ota_update_background)
pico_sign_binary(picow_ota_update_background)
# pico_set_binary_type(picow_ota_update_background no_flash)
# pico_package_uf2_output(picow_ota_update_background 0x10000000)

pico_add_extra_outputs(picow_ota_update_background)

add_executable(picow_ota_update_poll
        picow_ota_update.c
        )
target_compile_definitions(picow_ota_update_poll PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        PICO_CRT0_IMAGE_TYPE_TBYB=1
        )
target_include_directories(picow_ota_update_poll PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_ota_update_poll
        pico_cyw43_arch_lwip_poll
        pico_stdlib
        pico_sha256
        boot_uf2_headers
        )

pico_use_wifi_firmware_partition(picow_ota_update_poll)

pico_hash_binary(picow_ota_update_poll)
pico_sign_binary(picow_ota_update_poll)
# pico_set_binary_type(picow_ota_update_background no_flash)
# pico_package_uf2_output(picow_ota_update_background 0x10000000)

pico_add_extra_outputs(picow_ota_update_poll)
