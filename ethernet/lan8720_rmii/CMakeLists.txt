cmake_minimum_required(VERSION 3.12)

# rest of your project
add_executable(lan8720_rmii
    lan8720_rmii.c
)

target_link_libraries(lan8720_rmii pico_stdlib pico_multicore pico_rmii_ethernet)

# enable usb output, disable uart output
pico_enable_stdio_usb(lan8720_rmii 1)
pico_enable_stdio_uart(lan8720_rmii 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(lan8720_rmii)

add_subdirectory(pico-rmii-ethernet)