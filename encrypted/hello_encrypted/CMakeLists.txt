# Example encrypted binary - this should be secure against side channel attacks
add_executable(hello_encrypted
        hello_encrypted.c
        secret.S
        )

# include directory containing secret.txt
target_include_directories(hello_encrypted PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# add dependency on secret.txt
set_property(SOURCE secret.S APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/secret.txt)

# pull in common dependencies
target_link_libraries(hello_encrypted pico_stdlib)

# enable stdio_usb and stdio_uart
pico_enable_stdio_uart(hello_encrypted 1)
pico_enable_stdio_usb(hello_encrypted 1)

# set as no_flash binary
pico_set_binary_type(hello_encrypted no_flash)

# set version (optional)
pico_set_binary_version(hello_encrypted MAJOR 7 MINOR 3)

# set tbyb (optional)
# target_compile_definitions(hello_encrypted PRIVATE PICO_CRT0_IMAGE_TYPE_TBYB=1)

# configure otp output
pico_set_otp_key_output_file(hello_encrypted ${CMAKE_CURRENT_BINARY_DIR}/otp.json)

# sign, hash, and encrypt
pico_sign_binary(hello_encrypted ${CMAKE_CURRENT_LIST_DIR}/private.pem)
pico_hash_binary(hello_encrypted)
pico_encrypt_binary(hello_encrypted
    ${CMAKE_CURRENT_LIST_DIR}/privateaes.bin
    ${CMAKE_CURRENT_LIST_DIR}/ivsalt.bin
    EMBED)

# package uf2 in flash
pico_package_uf2_output(hello_encrypted)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hello_encrypted)

# add url via pico_set_program_url
example_auto_set_url(hello_encrypted)


# Example encrypted binary using MbedTLS - this is faster, but not secure against side channel attacks
add_executable(hello_encrypted_mbedtls
        hello_encrypted.c
        secret.S
        )

# include directory containing secret.txt
target_include_directories(hello_encrypted_mbedtls PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# pull in common dependencies
target_link_libraries(hello_encrypted_mbedtls pico_stdlib)

# enable stdio_usb and stdio_uart
pico_enable_stdio_uart(hello_encrypted_mbedtls 1)
pico_enable_stdio_usb(hello_encrypted_mbedtls 1)

# set as no_flash binary
pico_set_binary_type(hello_encrypted_mbedtls no_flash)

# set version (optional)
pico_set_binary_version(hello_encrypted_mbedtls MAJOR 7 MINOR 3)

# set tbyb (optional)
# target_compile_definitions(hello_encrypted_mbedtls PRIVATE PICO_CRT0_IMAGE_TYPE_TBYB=1)

# configure otp output
pico_set_otp_key_output_file(hello_encrypted_mbedtls ${CMAKE_CURRENT_BINARY_DIR}/otp_mbedtls.json)

# sign, hash, and encrypt using MbedTLS
pico_sign_binary(hello_encrypted_mbedtls ${CMAKE_CURRENT_LIST_DIR}/private.pem)
pico_hash_binary(hello_encrypted_mbedtls)
pico_encrypt_binary(hello_encrypted_mbedtls
    ${CMAKE_CURRENT_LIST_DIR}/privateaes.bin
    ${CMAKE_CURRENT_LIST_DIR}/ivsalt.bin
    EMBED
    MBEDTLS
    OTP_KEY_PAGE 29)

# package uf2 in flash
pico_package_uf2_output(hello_encrypted_mbedtls)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hello_encrypted_mbedtls)

# add url via pico_set_program_url
example_auto_set_url(hello_encrypted_mbedtls)
