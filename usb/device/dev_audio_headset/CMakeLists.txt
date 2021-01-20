set(CMAKE_C_FLAGS_DEBUG "-O0 -g")

add_executable(dev_audio_headset
        dev_audio_headset.c
        usb_descriptors.c
        )

target_include_directories(dev_audio_headset PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(dev_audio_headset PRIVATE pico_stdlib tinyusb_device tinyusb_board)
pico_add_extra_outputs(dev_audio_headset)

# add url via pico_set_program_url
example_auto_set_url(dev_audio_headset)
