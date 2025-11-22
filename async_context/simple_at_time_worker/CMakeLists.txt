add_executable(simple_at_time_worker
    simple_at_time_worker.c
    )

# common dependencies
target_link_libraries(simple_at_time_worker
    pico_stdlib
    pico_async_context_threadsafe_background
    )

# on WiFi boards we need `cyw43_arch_none` to control the on-board LED
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(simple_at_time_worker
        pico_cyw43_arch_none
        )
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(simple_at_time_worker)

# add url via pico_set_program_url
example_auto_set_url(simple_at_time_worker)
