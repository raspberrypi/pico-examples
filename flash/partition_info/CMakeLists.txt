add_executable(partition_info partition_info.c uf2_family_ids.c)

target_link_libraries(partition_info PRIVATE
        pico_stdlib
        pico_bootrom
        hardware_flash
        boot_uf2_headers
        )

# add a partition table
pico_embed_pt_in_binary(partition_info ${CMAKE_CURRENT_LIST_DIR}/pt.json)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(partition_info)

# add url via pico_set_program_url
example_auto_set_url(partition_info)
