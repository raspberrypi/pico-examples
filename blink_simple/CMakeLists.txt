cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../CMakeLists.txt)
    project(blink_simple C CXX ASM)
    include(../CMakeLists.txt)
endif()

add_executable(blink_simple
        blink_simple.c
)

# pull in common dependencies
target_link_libraries(blink_simple pico_stdlib)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(blink_simple)

# call pico_set_program_url to set path to example on github, so users can find the source for an example via picotool
example_auto_set_url(blink_simple)
