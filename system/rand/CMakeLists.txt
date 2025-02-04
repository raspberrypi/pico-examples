cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(rand C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(rand
        rand.c
        )
target_link_libraries(rand
        pico_stdlib
        pico_rand
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(rand)

# add url via pico_set_program_url
example_auto_set_url(rand)
