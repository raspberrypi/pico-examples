cmake_minimum_required(VERSION 3.12)

if (NOT CMAKE_PROJECT_NAME)
    include(../../CMakeLists.txt)
    project(microphone_adc C CXX ASM)
    include(../../CMakeLists.txt)
endif()

add_executable(microphone_adc
        microphone_adc.c
        )

# pull in common dependencies and adc hardware support
target_link_libraries(microphone_adc pico_stdlib hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(microphone_adc)

# add url via pico_set_program_url
example_auto_set_url(microphone_adc)
