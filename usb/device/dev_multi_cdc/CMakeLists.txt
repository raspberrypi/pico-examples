cmake_minimum_required(VERSION 3.13)

add_executable(dev_multi_cdc)

target_sources(dev_multi_cdc PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(dev_multi_cdc PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(dev_multi_cdc PUBLIC pico_stdlib pico_unique_id tinyusb_device tinyusb_board)

pico_enable_stdio_usb(dev_multi_cdc 1)
pico_add_extra_outputs(dev_multi_cdc)

# add url via pico_set_program_url
example_auto_set_url(dev_multi_cdc)
