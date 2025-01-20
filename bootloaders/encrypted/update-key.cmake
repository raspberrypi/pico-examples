if (CMAKE_VERSION VERSION_LESS 3.19)
    # Check if keyfile is not the default, and print warning
    file(READ ${CMAKE_CURRENT_LIST_DIR}/privateaes.bin key_file HEX)
    if (NOT ${key_file} STREQUAL "31b6d818232e7b7ca3b1b7907b2f41d251b50362d6210cb58d17e6d56b0d878d2b74a4bab91475889b052d3251c1350978bb6dc2bba65e95a22932345b2cd3f85de25f23eb27a4cdb08ef46e948619933ad89765292557654903fec6e98ba37e2b538068dd051017cac3a8048d12afd949a96d907cb3634f36c500b57174e69a")
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
