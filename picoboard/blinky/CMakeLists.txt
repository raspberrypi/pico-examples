add_executable(picoboard_blinky
        blinky.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(picoboard_blinky pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(picoboard_blinky)

# add url via pico_set_program_url
example_auto_set_url(picoboard_blinky)
