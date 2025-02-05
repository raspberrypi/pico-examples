cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    # Pull in SDK (must be before project)
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
        include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
    elseif (EXISTS ${CMAKE_CURRENT_LIST_DIR}/../../pico_sdk_import.cmake)
        # The root pico-examples one
        include(${CMAKE_CURRENT_LIST_DIR}/../../pico_sdk_import.cmake)
    else()
        message(FATAL_ERROR "Unable to locate pico_sdk_import.cmake - make sure you have copied it into this directory")
    endif()

    project(mbedtls_sha256 C CXX ASM)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    # Initialize the SDK
    pico_sdk_init()

    # Do not use this function when not part of main examples project
    function(example_auto_set_url)
    endfunction()
endif()

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
