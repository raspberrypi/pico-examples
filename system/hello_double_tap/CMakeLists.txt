add_executable(hello_double_tap
        hello_double_tap.c
        )

# Double tap reset into bootrom is injected by linking with the
# pico_bootsel_via_double_reset library
target_link_libraries(hello_double_tap
        pico_stdlib
        pico_bootsel_via_double_reset
        )

# Entering the bootloader in this way also lets us specify a GPIO to be used
# as a bootloader activity LED:
target_compile_definitions(hello_double_tap PRIVATE
	PICO_BOOTSEL_VIA_DOUBLE_RESET_ACTIVITY_LED=25
	)

pico_add_extra_outputs(hello_double_tap)

# add url via pico_set_program_url
example_auto_set_url(hello_double_tap)
