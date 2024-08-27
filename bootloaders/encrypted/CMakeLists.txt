# Encrypted Bootloader
add_executable(enc_bootloader
        enc_bootloader.c
        aes.S
        )

# pull in common dependencies
target_link_libraries(enc_bootloader pico_stdlib pico_rand)

# use stack guards, as AES variables are written near the stack
target_compile_definitions(enc_bootloader PRIVATE PICO_USE_STACK_GUARDS=1)

# set as no_flash binary
pico_set_binary_type(enc_bootloader no_flash)

# Add a linker script to run no_flash binary from anywhere
function(add_linker_script target origin length)
    # Write script file to run later, to generate the linker script
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/make_linker_script.cmake"
        "# create linker script to run from elsewhere in SRAM\n"
        "file(READ \${PICO_LINKER_SCRIPT_PATH}/memmap_no_flash.ld LINKER_SCRIPT)\n"
        "string(REPLACE \"RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 512k\" \"RAM(rwx) : ORIGIN =  \${origin}, LENGTH = \${length}\" LINKER_SCRIPT \"\${LINKER_SCRIPT}\")\n"
        "file(WRITE \${output_file} \"\${LINKER_SCRIPT}\")\n"
    )
    # Add command to run this whenever memmap_no_flash.ld changes
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}.ld
        COMMAND ${CMAKE_COMMAND}
            -DPICO_LINKER_SCRIPT_PATH:PATH=${PICO_LINKER_SCRIPT_PATH}
            -Dorigin="${origin}" -Dlength="${length}"
            -Doutput_file:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}/${target}.ld
            -P "${CMAKE_CURRENT_BINARY_DIR}/make_linker_script.cmake"
        DEPENDS ${PICO_LINKER_SCRIPT_PATH}/memmap_no_flash.ld)
    add_custom_target(${target}_ld DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}.ld)
    add_dependencies(${target} ${target}_ld)
    pico_set_linker_script(${target} ${CMAKE_CURRENT_BINARY_DIR}/${target}.ld)
endfunction()

# create linker script to run from 0x20070000
add_linker_script(enc_bootloader "0x20070000" "64k")

# configure otp output
configure_file(${CMAKE_CURRENT_LIST_DIR}/otp.json ${CMAKE_CURRENT_BINARY_DIR}/otp.json COPYONLY)
pico_set_otp_key_output_file(enc_bootloader ${CMAKE_CURRENT_BINARY_DIR}/otp.json)

# sign, hash, and clear SRAM
pico_sign_binary(enc_bootloader ${CMAKE_CURRENT_LIST_DIR}/private.pem)
pico_hash_binary(enc_bootloader)
pico_load_map_clear_sram(enc_bootloader)

# add partition table
pico_embed_pt_in_binary(enc_bootloader ${CMAKE_CURRENT_LIST_DIR}/enc-pt.json)

# create absolute uf2, and package in flash
pico_set_uf2_family(enc_bootloader "absolute")
pico_package_uf2_output(enc_bootloader 0x10000000)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(enc_bootloader)

# add url via pico_set_program_url
example_auto_set_url(enc_bootloader)



# Example binary to load
add_executable(hello_serial_enc
        hello_serial.c
        )

# pull in common dependencies
target_link_libraries(hello_serial_enc pico_stdlib)

# set as no_flash binary
pico_set_binary_type(hello_serial_enc no_flash)

# create linker script to ensure it doesn't overwrite the bootloader at 0x20070000
add_linker_script(hello_serial_enc "0x20000000" "448k")

# sign, hash, and encrypt
pico_sign_binary(hello_serial_enc ${CMAKE_CURRENT_LIST_DIR}/private.pem)
pico_hash_binary(hello_serial_enc)
pico_encrypt_binary(hello_serial_enc ${CMAKE_CURRENT_LIST_DIR}/privateaes.bin)

# package uf2 in flash
pico_package_uf2_output(hello_serial_enc 0x10000000)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hello_serial_enc)

# add url via pico_set_program_url
example_auto_set_url(hello_serial_enc)
