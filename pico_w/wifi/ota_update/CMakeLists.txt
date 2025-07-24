add_executable(picow_ota_update
        picow_ota_update.c
        )
target_compile_definitions(picow_ota_update PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        PICO_CRT0_IMAGE_TYPE_TBYB=1
        )
target_include_directories(picow_ota_update PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        )
target_link_libraries(picow_ota_update
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_sha256
        boot_uf2_headers
        )

pico_use_wifi_firmware_partition(picow_ota_update)

pico_hash_binary(picow_ota_update)
pico_sign_binary(picow_ota_update ${CMAKE_CURRENT_LIST_DIR}/private.pem)

# By default this example requires a partition table in flash, and will
# update the partition that is not currently in use. To use it without
# a partition table in flash, uncomment the following lines to make it
# a no_flash binary, so it can update the currently running program.

# pico_set_binary_type(picow_ota_update no_flash)
# pico_package_uf2_output(picow_ota_update 0x10000000)

pico_add_extra_outputs(picow_ota_update)
