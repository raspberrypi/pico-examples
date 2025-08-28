# Tell CMake where to find the executable source file
add_executable(gps_uart gps_uart.c)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(gps_uart pico_stdlib hardware_uart)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(gps_uart)