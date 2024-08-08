if (NOT TARGET hardware_sha256)
    return()
endif()

# This example uses the mbedtls SHA-256 API
# mbedtls_config.h defines MBEDTLS_SHA256_ALT enabling hardware acceleration if supported
add_executable(mbedtls_sha256
        mbedtls_sha256.c
        )
target_link_libraries(mbedtls_sha256
        pico_stdlib
        pico_mbedtls
)
target_include_directories(mbedtls_sha256 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)
pico_add_extra_outputs(mbedtls_sha256)
example_auto_set_url(mbedtls_sha256)
