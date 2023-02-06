cmake_minimum_required(VERSION 3.13)

add_executable(host_cdc_msc_hid)

# Example source
target_sources(host_cdc_msc_hid PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/hid_app.c
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/msc_app.c
        ${CMAKE_CURRENT_LIST_DIR}/cdc_app.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(host_cdc_msc_hid PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_host
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(host_cdc_msc_hid PUBLIC pico_stdlib tinyusb_host tinyusb_board)

pico_add_extra_outputs(host_cdc_msc_hid)

# add url via pico_set_program_url
example_auto_set_url(host_cdc_msc_hid)
