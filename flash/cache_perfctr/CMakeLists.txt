add_executable(flash_cache_perfctr
        flash_cache_perfctr.c
        )

target_link_libraries(flash_cache_perfctr
        pico_stdlib
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(flash_cache_perfctr)

# add url via pico_set_program_url
example_auto_set_url(flash_cache_perfctr)
