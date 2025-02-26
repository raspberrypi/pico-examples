# Example encrypted binard
add_executable(hello_encrypted
        hello_encrypted.c
        )

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

# Copy otp.json file to build directory
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/otp.json
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/otp.json" "${CMAKE_CURRENT_BINARY_DIR}/otp.json"
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/otp.json)
add_custom_target(hello_encrypted_otp_json DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/otp.json)
add_dependencies(hello_encrypted hello_encrypted_otp_json)

# configure otp output
pico_set_otp_key_output_file(hello_encrypted ${CMAKE_CURRENT_BINARY_DIR}/otp.json)

# sign, hash, and encrypt
pico_sign_binary(hello_encrypted ${CMAKE_CURRENT_LIST_DIR}/private.pem)
pico_hash_binary(hello_encrypted)
pico_encrypt_binary(hello_encrypted
    ${CMAKE_CURRENT_LIST_DIR}/privateaes.bin
    ${CMAKE_CURRENT_LIST_DIR}/ivsalt.bin
    EMBED
    OTP_KEY_PAGE 29)

# package uf2 in flash
pico_package_uf2_output(hello_encrypted 0x10000000)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hello_encrypted)

# add url via pico_set_program_url
example_auto_set_url(hello_encrypted)
