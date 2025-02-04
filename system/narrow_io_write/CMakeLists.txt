cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(narrow_io_write C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(narrow_io_write
        narrow_io_write.c
        )

# pull in common dependencies
target_link_libraries(narrow_io_write pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(narrow_io_write)

# add url via pico_set_program_url
example_auto_set_url(narrow_io_write)
