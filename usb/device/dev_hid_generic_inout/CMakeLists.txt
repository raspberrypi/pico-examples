add_executable(dev_hid_generic_inout
        dev_hid_generic_inout.c
        usb_descriptors.c
        )

target_include_directories(dev_hid_generic_inout PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(dev_hid_generic_inout PRIVATE pico_stdlib tinyusb_device tinyusb_board)
pico_add_extra_outputs(dev_hid_generic_inout)

# add url via pico_set_program_url
example_auto_set_url(dev_hid_generic_inout)
