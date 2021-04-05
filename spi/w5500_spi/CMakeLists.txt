add_executable(w5500_spi
        w5500_spi.c
        loopback.c socket.c w5500.c wizchip_conf.c
        )
#add_library(loopback.c socket.c w5500.c wizchip_conf.c)
# Pull in our (to be renamed) simple get you started dependencies
target_link_libraries(w5500_spi pico_stdlib hardware_spi) 

# create map/bin/hex file etc.
pico_add_extra_outputs(w5500_spi)

# add url via pico_set_program_url
example_auto_set_url(w5500_spi)
