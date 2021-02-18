add_executable(dev_midi
        dev_midi.c
        usb_descriptors.c
        )

target_include_directories(dev_midi PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(dev_midi PRIVATE pico_stdlib tinyusb_device tinyusb_board)
pico_add_extra_outputs(dev_midi)

# add url via pico_set_program_url
# example_auto_set_url(dev_midi)