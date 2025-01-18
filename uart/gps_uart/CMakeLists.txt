# Tell CMake where to find the executable source file
add_executable(gps_uart gps_uart.c)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(gps_uart pico_stdlib hardware_uart)

# Enable usb output, disable uart output
pico_enable_stdio_usb(gps_uart 1)
pico_enable_stdio_uart(gps_uart 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(gps_uart)