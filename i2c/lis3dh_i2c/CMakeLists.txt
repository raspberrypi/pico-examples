cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(lis3dh_i2c C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(lis3dh_i2c
        lis3dh_i2c.c
        )

# pull in common dependencies and additional i2c hardware support
target_link_libraries(lis3dh_i2c pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(lis3dh_i2c)

# add url via pico_set_program_url
example_auto_set_url(lis3dh_i2c)
