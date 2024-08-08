add_executable(dvi_out_hstx_encoder
        dvi_out_hstx_encoder.c
        )

target_include_directories(dvi_out_hstx_encoder PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/images
        )

# pull in common dependencies
target_link_libraries(dvi_out_hstx_encoder
        pico_stdlib
        pico_multicore
        hardware_dma
        pico_sync
        )

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(dvi_out_hstx_encoder)
