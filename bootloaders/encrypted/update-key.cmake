if (CMAKE_VERSION VERSION_LESS 3.19)
    # Check if keyfile is not the default, and print warning
    file(READ ${CMAKE_CURRENT_LIST_DIR}/privateaes.bin key_file HEX)
    if (NOT ${key_file} STREQUAL "000102030405060708090a0b0c0d0e0f00102030405060708090a0b0c0d0e0f0")
        message(WARNING
            "Encrypted bootloader AES key not updated in otp.json file, as CMake version is < 3.19"
            " - you will need to change the key in otp.json manually and re-run the build"
        )
    endif()
else()
    # Read the JSON file.
    file(READ ${CMAKE_CURRENT_LIST_DIR}/otp.json json_string)
    # Read the key file
    file(READ ${CMAKE_CURRENT_LIST_DIR}/privateaes.bin key_file HEX)

    # adds '0x' prefix, comma suffix, and quotes for every byte
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "\"0x\\1\", " key_file ${key_file})
    set(key_file_json "[${key_file}]")

    string(JSON json_string SET ${json_string} "30:0" "value" ${key_file_json})

    file(WRITE ${CMAKE_CURRENT_LIST_DIR}/otp.json ${json_string})
endif()
