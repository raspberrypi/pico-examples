add_executable(christmas_melody
    christmas_melody.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(christmas_melody pico_stdlib pico_tone)

# Create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(christmas_melody)

# add url via pico_set_program_url
example_auto_set_url(christmas_melody)
