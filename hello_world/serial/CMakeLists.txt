cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(serial C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(hello_serial
        hello_serial.c
        )

# pull in common dependencies
target_link_libraries(hello_serial pico_stdlib)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(hello_serial)

# add url via pico_set_program_url
example_auto_set_url(hello_serial)
