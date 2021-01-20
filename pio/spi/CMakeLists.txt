add_executable(pio_spi_flash)

pico_generate_pio_header(pio_spi_flash ${CMAKE_CURRENT_LIST_DIR}/spi.pio)

target_sources(pio_spi_flash PRIVATE
        spi_flash.c
        pio_spi.c
        pio_spi.h
        )

target_link_libraries(pio_spi_flash PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_spi_flash)

example_auto_set_url(pio_spi_flash)

add_executable(pio_spi_loopback)

pico_generate_pio_header(pio_spi_loopback ${CMAKE_CURRENT_LIST_DIR}/spi.pio)

target_sources(pio_spi_loopback PRIVATE
        spi_loopback.c
        pio_spi.c
        pio_spi.h
        )

target_link_libraries(pio_spi_loopback PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_spi_loopback)

example_auto_set_url(pio_spi_loopback)
