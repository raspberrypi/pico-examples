cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(spi_lcd C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(hstx_spi_lcd
        hstx_spi_lcd.c
        )

target_include_directories(hstx_spi_lcd PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/images
        )

# pull in common dependencies
target_link_libraries(hstx_spi_lcd
        pico_stdlib
        pico_sync
        )

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hstx_spi_lcd)
