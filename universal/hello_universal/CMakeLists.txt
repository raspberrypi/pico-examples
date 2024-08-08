add_executable(hello_universal
        hello_universal.c
        )

# pull in common dependencies
target_link_libraries(hello_universal pico_stdlib)

# enable usb output and uart output
pico_enable_stdio_uart(hello_universal 1)
pico_enable_stdio_usb(hello_universal 1)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hello_universal)
