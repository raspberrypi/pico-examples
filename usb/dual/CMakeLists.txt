set(FAMILY rp2040)
set(BOARD pico_sdk)
set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_dual_")
add_subdirectory(${PICO_TINYUSB_PATH}/examples/dual tinyusb_dual_examples)