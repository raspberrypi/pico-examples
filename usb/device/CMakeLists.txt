set(FAMILY rp2040)
set(BOARD pico_sdk)
set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_dev_")
# Provide an LWIP path for net_lwip_webserver in case the module does not exist in tinyusb
set(TINYUSB_LWIP_PATH ${PICO_LWIP_PATH})
# Some examples use this, and we need to set this here due to a bug in the TinyUSB CMake config
set(TOP ${PICO_TINYUSB_PATH})
add_subdirectory(${PICO_TINYUSB_PATH}/examples/device tinyusb_device_examples)
add_subdirectory_exclude_platforms(dev_hid_composite)
add_subdirectory_exclude_platforms(dev_lowlevel)
add_subdirectory_exclude_platforms(dev_multi_cdc)
