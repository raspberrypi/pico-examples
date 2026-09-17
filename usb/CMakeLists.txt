# This is not set by default in latest TinyUSB
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

# This function is added in later TinyUSB, but requires
# tools/get_deps.py to have been run within TinyUSB.
# It just prints out code sizes which isn't really needed
# here, so just make it a no-op
function(family_add_linkermap TARGET)
endfunction()

# This is used to filter the tinyusb examples
# Ignore examples that don't build properly without extra code
# Ignore freertos examples unless FREERTOS_KERNEL_PATH is defined
function(family_add_subdirectory DIR)
    family_filter(SHOULD_ADD "${DIR}")
    if (SHOULD_ADD)
        if (NOT IS_DIRECTORY ${PICO_TINYUSB_PATH}/lib/fatfs)
            list(APPEND tinyusb_example_ignore_list
                msc_file_explorer msc_file_explorer_freertos # need fatfs repo
            )
        endif()
        if (NOT TARGET FreeRTOS-Kernel)
            # skip freertos examples
            list(APPEND tinyusb_example_ignore_list
                ${tinyusb_freertos_examples}
            )
        endif()
        list(FIND tinyusb_example_ignore_list ${DIR} skip_pos)
        if (skip_pos LESS 0)
            add_subdirectory(${DIR})
        endif()
    endif()
endfunction()

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
