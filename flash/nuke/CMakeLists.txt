add_executable(flash_nuke
        nuke.c
        )

target_link_libraries(flash_nuke
        pico_stdlib
        hardware_flash
        )

# We use flash command 0x9f to dynamically get the flash size
# If that fails, assume 16Mb
math(EXPR FLASH_SIZE_OVERRIDE "16 * 1024 * 1024" OUTPUT_FORMAT DECIMAL)
target_compile_definitions(flash_nuke PRIVATE
        PICO_FLASH_SIZE_BYTES=${FLASH_SIZE_OVERRIDE}
        )

# It doesn't make sense to run this program from flash. Always build a
# RAM-only binary.
pico_set_binary_type(flash_nuke no_flash)

pico_add_extra_outputs(flash_nuke)

# add url via pico_set_program_url
example_auto_set_url(flash_nuke)

