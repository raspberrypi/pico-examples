add_executable(partition_info partition_info.c)
target_link_libraries(partition_info PRIVATE
  pico_stdlib
  pico_bootrom
)

pico_embed_pt_in_binary(partition_info ${CMAKE_CURRENT_LIST_DIR}/pt.json)
pico_enable_stdio_usb(partition_info 1)
pico_add_extra_outputs(partition_info)
example_auto_set_url(partition_info)

