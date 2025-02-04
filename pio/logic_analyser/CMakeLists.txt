cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(logic_analyser C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(pio_logic_analyser)

target_sources(pio_logic_analyser PRIVATE logic_analyser.c)

target_link_libraries(pio_logic_analyser PRIVATE pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(pio_logic_analyser)

# add url via pico_set_program_url
example_auto_set_url(pio_logic_analyser)