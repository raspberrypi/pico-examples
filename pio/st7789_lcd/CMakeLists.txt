add_executable(pio_st7789_lcd)

pico_generate_pio_header(pio_st7789_lcd ${CMAKE_CURRENT_LIST_DIR}/st7789_lcd.pio)

target_sources(pio_st7789_lcd PRIVATE st7789_lcd.c)

target_link_libraries(pio_st7789_lcd PRIVATE pico_stdlib hardware_pio hardware_interp)
pico_add_extra_outputs(pio_st7789_lcd)

# add url via pico_set_program_url
example_auto_set_url(pio_st7789_lcd)
