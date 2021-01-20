add_executable(hello_double_tap
        hello_double_tap.c
        )

# Double tap reset into bootrom is injected by linking with the double_tap_usb_boot library
target_link_libraries(hello_double_tap
        pico_stdlib
        double_tap_usb_boot
        )

pico_add_extra_outputs(hello_double_tap)

# add url via pico_set_program_url
example_auto_set_url(hello_double_tap)
