if (NOT PICO_NO_HARDWARE)
    add_subdirectory(w5500_spi)
    add_subdirectory(lan8720_rmii)
endif ()
