cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(multicore_runner_queue C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(multicore_runner_queue
        multicore_runner_queue.c
        )

target_link_libraries(multicore_runner_queue
        pico_multicore
        pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(multicore_runner_queue)

# add url via pico_set_program_url
example_auto_set_url(multicore_runner_queue)