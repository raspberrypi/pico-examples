add_executable(pio_7_segment)

target_sources(pio_7_segment PRIVATE 7_segment.c)

add_subdirectory(pio_7_segment_library)

target_link_libraries(pio_7_segment PRIVATE
	pico_stdlib
	hardware_pio
	pio_7_segment_library)

pico_add_extra_outputs(pio_7_segment)

example_auto_set_url(pio_7_segment)
