add_executable(runtime_flash_permissions
        runtime_flash_permissions.c
        )

# pull in common dependencies
target_link_libraries(runtime_flash_permissions pico_stdlib)

# not necessary for the example functionality, but shows you how you would add a partition table
pico_embed_pt_in_binary(runtime_flash_permissions ${CMAKE_CURRENT_LIST_DIR}/pt.json)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(runtime_flash_permissions)

# add url via pico_set_program_url
example_auto_set_url(runtime_flash_permissions)
