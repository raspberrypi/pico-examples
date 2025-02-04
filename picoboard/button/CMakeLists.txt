cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(button C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(picoboard_button
        button.c
        )

# pull in common dependencies
target_link_libraries(picoboard_button pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(picoboard_button)

# add url via pico_set_program_url
example_auto_set_url(picoboard_button)
