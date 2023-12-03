add_executable(tone_generation
    tone_generation.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(tone_generation pico_stdlib hardware_pwm)

# Create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(tone_generation)

# add url via pico_set_program_url
example_auto_set_url(tone_generation)
