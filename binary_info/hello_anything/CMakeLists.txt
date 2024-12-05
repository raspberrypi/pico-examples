if (TARGET tinyusb_device)
    add_executable(hello_anything
            hello_anything.c
            )

    # pull in common dependencies
    target_link_libraries(hello_anything pico_stdlib)

    # enable usb and uart output
    pico_enable_stdio_usb(hello_anything 1)
    pico_enable_stdio_uart(hello_anything 1)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(hello_anything)

    # add url via pico_set_program_url
    example_auto_set_url(hello_anything)
elseif(PICO_ON_DEVICE)
    message("Skipping hello_anything because TinyUSB submodule is not initialized in the SDK")
endif()
