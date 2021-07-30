add_executable(narrow_io_write
        narrow_io_write.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(narrow_io_write pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(narrow_io_write)

# add url via pico_set_program_url
example_auto_set_url(narrow_io_write)
