cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../../CMakeLists.txt)
    project(dev_lowlevel C CXX ASM)
    include(../../../CMakeLists.txt)
endif()

add_executable(dev_lowlevel
        dev_lowlevel.c
        )

target_link_libraries(dev_lowlevel PRIVATE pico_stdlib hardware_resets hardware_irq)
pico_add_extra_outputs(dev_lowlevel)

# add url via pico_set_program_url
example_auto_set_url(dev_lowlevel)
