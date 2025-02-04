cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(addition C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(pio_addition)

pico_generate_pio_header(pio_addition ${CMAKE_CURRENT_LIST_DIR}/addition.pio)

target_sources(pio_addition PRIVATE addition.c)

target_link_libraries(pio_addition PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(pio_addition)

# add url via pico_set_program_url
example_auto_set_url(pio_addition)
