if (TARGET tinyusb_device)
    add_executable(picotool_reset
            picotool_reset.c
            )

    target_compile_definitions(picotool_reset PRIVATE
	    PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=1
	    PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR=1
    )

    # pull in common dependencies
    target_link_libraries(picotool_reset pico_stdlib)

    # enable usb interface, disable uart output
    pico_enable_stdio_usb(picotool_reset 1)
    pico_enable_stdio_uart(picotool_reset 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(picotool_reset)

    # add url via pico_set_program_url
    example_auto_set_url(picotool_reset)
elseif(PICO_ON_DEVICE)
    message("Skipping picotool_reset because TinyUSB submodule is not initialized in the SDK")
endif()
