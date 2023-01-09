if (TARGET tinyusb_device)
    add_executable(pwm_led_fade
            pwm_led_fade.c
            )

    target_compile_definitions(pwm_led_fade PUBLIC PICO_DEFAULT_LED_PIN=0)

    # pull in common dependencies
    target_link_libraries(pwm_led_fade pico_stdlib hardware_pwm hardware_dma)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(pwm_led_fade 1)
    pico_enable_stdio_uart(pwm_led_fade 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(pwm_led_fade)

    # add url via pico_set_program_url
    example_auto_set_url(pwm_led_fade)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building pwm_led_fade because TinyUSB submodule is not initialized in the SDK")
endif()
