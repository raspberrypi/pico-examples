set(FAMILY rp2040)
set(BOARD pico_sdk)
set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_dev_")
add_subdirectory(${PICO_TINYUSB_PATH}/examples/device tinyusb_device_examples)

add_subdirectory(dev_hid_composite)
add_subdirectory(dev_lowlevel)
