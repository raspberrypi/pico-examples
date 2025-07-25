add_executable(st7789_display_spi
        st7789_display_demo.c
        fonts.c
)

# pull in common dependencies and additional spi hardware support
target_link_libraries(st7789_display_spi
 pico_stdlib
 hardware_spi
 )

# create map/bin/hex file etc.
pico_add_extra_outputs(st7789_display_spi)
