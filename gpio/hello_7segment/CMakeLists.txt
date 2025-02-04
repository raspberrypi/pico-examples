cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(hello_7segment C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(hello_7segment
        hello_7segment.c
        )

# pull in common dependencies
target_link_libraries(hello_7segment pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_7segment)

# add url via pico_set_program_url
example_auto_set_url(hello_7segment)
