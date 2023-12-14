add_executable(debounce_gpio_irq
        debounce_gpio_irq.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(debounce_gpio_irq pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(debounce_gpio_irq)

# add url via pico_set_program_url
example_auto_set_url(debounce_gpio_irq)