cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(multicore_doorbell C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(multicore_doorbell
        multicore_doorbell.c
        )
target_link_libraries(multicore_doorbell
        pico_stdlib
        pico_multicore)
pico_add_extra_outputs(multicore_doorbell)
example_auto_set_url(multicore_doorbell)
