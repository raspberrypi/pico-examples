add_library(double_tap_usb_boot INTERFACE)

# inclusion of this library will allow you to double tap reset within 100ms to launch into bootrom USB bootloader
if (PICO_ON_DEVICE)
    target_sources(double_tap_usb_boot INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/double_tap_usb_boot.c
            )

    target_link_libraries(double_tap_usb_boot INTERFACE
            pico_bootrom
            pico_time
            )
endif ()