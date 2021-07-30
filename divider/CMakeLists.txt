add_executable(hello_divider
        hello_divider.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(hello_divider pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_divider)

# add url via pico_set_program_url
example_auto_set_url(hello_divider)