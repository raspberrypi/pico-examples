if (NOT PICO_NO_HARDWARE)
    add_subdirectory(spi_master)
    add_subdirectory(spi_slave)
endif ()
