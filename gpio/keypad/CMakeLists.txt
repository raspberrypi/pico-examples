set(TARGET_NAME keypad)
add_executable(${TARGET_NAME}
    keypad.c
    )

# pull in common dependencies
target_link_libraries(keypad pico_stdlib)

# Enable printing via USB serial
pico_enable_stdio_usb(${TARGET_NAME} 1)
pico_enable_stdio_uart(${TARGET_NAME} 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(${TARGET_NAME})

# add url via pico_set_program_url
example_auto_set_url(${TARGET_NAME})
