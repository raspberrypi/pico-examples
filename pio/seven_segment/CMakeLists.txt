add_executable(pio_seven_segment)

target_sources(pio_seven_segment PRIVATE 7_segment.c)

add_subdirectory_exclude_platforms(pio_7_segment_library)

target_link_libraries(pio_seven_segment PRIVATE
	pico_stdlib 
	hardware_pio 
	pio_7_segment_library
	)

pico_add_extra_outputs(pio_seven_segment)

# add url via pico_set_program_url
example_auto_set_url(pio_seven_segment)