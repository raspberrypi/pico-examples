add_executable(dev_lowlevel
        dev_lowlevel.c
        )

target_link_libraries(dev_lowlevel PRIVATE pico_stdlib hardware_resets hardware_irq)
pico_add_extra_outputs(dev_lowlevel)

# add url via pico_set_program_url
example_auto_set_url(dev_lowlevel)
