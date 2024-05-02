add_executable(tdm_simple
        tdm.c
        ssd1306.c
        )


target_compile_definitions(tdm_simple PRIVATE
        PICO_DEFAULT_UART_TX_PIN=12
        PICO_DEFAULT_UART_RX_PIN=13
        PICO_DEFAULT_I2C_SDA_PIN=24
        PICO_DEFAULT_I2C_SCL_PIN=25 )

pico_generate_pio_header(tdm_simple ${CMAKE_CURRENT_LIST_DIR}/tdm.pio)

target_sources(tdm_simple PRIVATE
        tdm.c
        ssd1306.c
        crctab.c
        zmodem.c
        hw_config.c
        pio_tdm.c
        pio_tdm.h
        )

      
      
# pull in common dependencies and additional i2c hardware support
target_link_libraries(tdm_simple pico_stdlib hardware_i2c hardware_dma hardware_pio FatFs_SPI)

# enable usb output, disable uart output
pico_enable_stdio_usb(tdm_simple 1)
pico_enable_stdio_uart(tdm_simple 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(tdm_simple)

# add url via pico_set_program_url
example_auto_set_url(tdm_simple)
