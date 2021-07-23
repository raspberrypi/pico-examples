#[[add_executable(w5500_spi
        w5500_spi.c
        loopback.c socket.c w5500.c wizchip_conf.c
        )
#add_library(loopback.c socket.c w5500.c wizchip_conf.c)
# Pull in our (to be renamed) simple get you started dependencies
target_link_libraries(w5500_spi pico_stdlib hardware_spi) 

# create map/bin/hex file etc.
pico_add_extra_outputs(w5500_spi)

# add url via pico_set_program_url
example_auto_set_url(w5500_spi)]]#

set (ETHERNET /home/pi/Documents/ioLibrary_Driver-master/Ethernet)

add_subdirectory(${ETHERNET} ${CMAKE_BINARY_DIR}/eth)

add_executable(w5500_spi w5500_spi.c 
w5500.c loopback.c wizchip_conf.c)

pico_enable_stdio_uart(w5500_spi 1)
pico_enable_stdio_usb(w5500_spi 0)

target_link_libraries(w5500_spi PUBLIC pico_stdio hardware_spi socket)

pico_add_extra_outputs(w5500_spi)

# add url via pico_set_program_url
example_auto_set_url(w5500_spi)
