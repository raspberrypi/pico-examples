# family.cmake defines TINYUSB_OPT_OS and is included twice...
# 1/ the first time by pico-sdk in src/rp2_common/tinyusb/CMakeLists.txt
# 2/ before each tinyusb example via family_support.cmake
# family.cmake has include_guard(GLOBAL) so the second include doesn't happen
# But each example calls the cmake function family_configure_device_example
# as the second include didn't happen TINYUSB_OPT_OS has gone out of scope
# So make sure it's defined to avoid a redefinition with an invalid value
# For freertos examples TINYUSB_OPT_OS is actually defined in FreeRTOSConfig.h
set(TINYUSB_OPT_OS OPT_OS_PICO)

# List of freertos examples
set(tinyusb_freertos_examples
    audio_4_channel_mic_freertos
    audio_test_freertos
    cdc_msc_freertos
    hid_composite_freertos
    midi_test_freertos
    msc_file_explorer_freertos
)

if (TARGET tinyusb_device)
    add_subdirectory(device)
else ()
    message("Skipping TinyUSB device examples as TinyUSB is unavailable")
endif ()
if (TARGET tinyusb_host)
    add_subdirectory(host)
else ()
    message("Skipping TinyUSB host examples as TinyUSB is unavailable")
endif ()
if (TARGET tinyusb_pico_pio_usb)
    add_subdirectory(dual)
else ()
    message("Skipping TinyUSB dual examples, as TinyUSB hw/mcu/raspberry_pi/Pico-PIO-USB submodule unavailable")
endif ()

# tinyusb freertos examples want to use FREERTOS_CONFIG_FILE_DIRECTORY to set the freertos config directory
# we have other freertos examples and we want the config to be under the control of each example
# We prefer to add the freertos config directory to the include directory list
# Add the directory now for the tinyusb freertos examples
foreach(tinyusb_freertos_example ${tinyusb_freertos_examples})
    if (TARGET ${tinyusb_freertos_example})
        target_include_directories(${tinyusb_freertos_example} PRIVATE
            ${PICO_TINYUSB_PATH}/hw/bsp/rp2040/FreeRTOSConfig
        )
    endif()
endforeach()
