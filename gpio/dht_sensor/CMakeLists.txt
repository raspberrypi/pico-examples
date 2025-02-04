cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(dht_sensor C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(dht
        dht.c
        )

target_link_libraries(dht pico_stdlib)

pico_add_extra_outputs(dht)

# add url via pico_set_program_url
example_auto_set_url(dht)

