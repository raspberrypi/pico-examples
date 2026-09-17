# Add secure executable
add_executable(tls_verify_secure secure.c)
target_link_libraries(tls_verify_secure
        pico_stdlib
        hardware_watchdog
        pico_secure
)

pico_set_binary_type(tls_verify_secure no_flash)
pico_package_uf2_output(tls_verify_secure 0x10000000)
pico_add_extra_outputs(tls_verify_secure)

# Add non-secure executable
add_executable(tls_verify_nonsecure nonsecure.c tls_common.c)
pico_set_uf2_family(tls_verify_nonsecure "rp2350-arm-ns")
target_link_libraries(tls_verify_nonsecure
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mbedtls
        pico_mbedtls
)
target_include_directories(tls_verify_nonsecure PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_definitions(tls_verify_nonsecure PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"

    # By default verification is optional (MBEDTLS_SSL_VERIFY_OPTIONAL)
    # Make it required for this test
    ALTCP_MBEDTLS_AUTHMODE=MBEDTLS_SSL_VERIFY_REQUIRED
)

pico_add_extra_outputs(tls_verify_nonsecure)

# Set security options
pico_set_security_options(tls_verify_secure tls_verify_nonsecure
    STDIO RAND DMA USER_IRQ PIO GPIO USB
    NONSECURE_TIMER 1
)

# Set security ram split
pico_set_security_ram_split(tls_verify_secure tls_verify_nonsecure SIMPLE 64k 2k)

# Create concatenated UF2, for loading into the partition table using drag & drop (non-secure first)
pico_concatenate_uf2_outputs(tls_verify_trustzone_drag_drop tls_verify_nonsecure tls_verify_secure)

# Create combined UF2 containing partition table, for loading using picotool or drag & drop
pico_combine_uf2_outputs(tls_verify_trustzone_combined trustzone_pt
        trustzone_none tls_verify_secure trustzone_none tls_verify_nonsecure
        PARTITIONS 0 1 2 3)

# Create combined CMake target, for easy building
add_custom_target(tls_verify_trustzone ALL
        DEPENDS tls_verify_trustzone_combined tls_verify_trustzone_drag_drop)
