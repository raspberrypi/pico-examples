set(FAMILY rp2040)
set(BOARD pico_sdk)
set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_host_")
add_subdirectory(${PICO_TINYUSB_PATH}/examples/host tinyusb_host_examples)

add_subdirectory(host_cdc_msc_hid)