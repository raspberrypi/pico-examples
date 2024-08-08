# todo peter fix mixture of sha256 and MBED
if (TARGET pico_sha256 AND TARGET pico_mbedtls)
    add_subdirectory_exclude_platforms(sha256)
    add_subdirectory_exclude_platforms(mbedtls_sha256)
else()
    message("Skipping SHA256 examples as pico_sha256 or pico_mbedtls unavailable")
endif ()
