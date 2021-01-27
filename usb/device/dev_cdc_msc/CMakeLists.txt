add_executable(dev_cdc_msc
        dev_cdc_msc.c
        msc_disk.c
        usb_descriptors.c
        )

target_include_directories(dev_cdc_msc PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(dev_cdc_msc PRIVATE pico_stdlib tinyusb_device tinyusb_board)
pico_add_extra_outputs(dev_cdc_msc)

# add url via pico_set_program_url
example_auto_set_url(dev_cdc_msc)
