add_executable(hello_usb
        hello_usb.c
        )

# Pull in our pico_stdlib which aggregates commonly used features
target_link_libraries(hello_usb pico_stdlib)

# enable usb output, disable uart output
pico_enable_stdio_usb(hello_usb 1)
pico_enable_stdio_uart(hello_usb 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hello_usb)

# add url via pico_set_program_url
example_auto_set_url(hello_usb)
