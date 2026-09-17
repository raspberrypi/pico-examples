# Common partition table UF2
pico_create_partition_table_uf2(trustzone_pt ${CMAKE_CURRENT_LIST_DIR}/trustzone_pt.json)

# Common partition erase UF2, to erase the other partitions and ensure a clean start
pico_create_erase_partition_uf2(trustzone_none)

add_subdirectory_exclude_platforms(hello_trustzone rp2040 rp2350-riscv)

if (PICO_CYW43_SUPPORTED) # set by PICO_BOARD=pico_w
    add_subdirectory_exclude_platforms(wifi_tls_verify rp2040 rp2350-riscv)
endif()
