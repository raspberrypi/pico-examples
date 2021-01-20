add_executable(host_hid
        host_hid.c
        )

target_include_directories(host_hid PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(host_hid PRIVATE pico_stdlib tinyusb_host tinyusb_board)
pico_add_extra_outputs(host_hid)

# add url via pico_set_program_url
example_auto_set_url(host_hid)
